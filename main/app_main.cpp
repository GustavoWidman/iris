// ESP-Matter example initialization references: Public Domain / CC0 upstream.
// Original bridge logic: MIT. See THIRD_PARTY.md for license scope.
#include <atomic>
#include <cstring>
#include <cstdio>
#include <esp_log.h>
#include <esp_timer.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <esp_wifi.h>
#include <esp_matter.h>
#include <esp_matter_providers.h>
#include <platform/CHIPDeviceLayer.h>
#include <platform/ESP32/NetworkCommissioningDriver.h>
#include <app/server/Server.h>
#include <app/server/CommissioningWindowManager.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
extern "C" {
#include <host/ble_hs.h>
#include <host/util/util.h>
}
#include "color_math.h"
#include "bj_led_protocol.h"
#include "console_command.h"
#include "led_profile_config.h"
#include "pairing_provider.h"
using namespace esp_matter;
using namespace chip::app::Clusters;
static const char *TAG="BJ_BRIDGE";
struct Desired {bool on=false;uint8_t level=13,h=0,s=0,mode=1;uint16_t x=20495,y=21561,mired=250;};
static Desired desired;
// Identity unless the explicit experimental build option is enabled.
static const LedCalibration led_calibration=firmware_led_calibration();
static portMUX_TYPE state_lock=portMUX_INITIALIZER_UNLOCKED;
static uint32_t revision=0;
// Counts only Matter-backed desired-state changes. Exact RGB test requests are never persisted.
static uint32_t persistent_revision=0;
static ExactRgbOverride exact_override;
static bool saved_state=false;
static std::atomic<uint16_t> connection{BLE_HS_CONN_HANDLE_NONE},write_handle{0};
static std::atomic<bool> connecting{false};
static uint16_t endpoint_id;
static PairingProvider pairing;
static uint8_t own_type;
static std::atomic<int64_t> next_attempt{0};
static std::atomic<uint32_t> successful_writes{0},failed_writes{0};
static bool exact_target(const ble_gap_event *e,const ble_hs_adv_fields &f){
 if(!f.name||f.name_len!=6||memcmp(f.name,"BJ_LED",6))return false;
#ifdef BRIDGE_LED_ADDR
 const uint8_t address[]=BRIDGE_LED_ADDR;
 return memcmp(address,e->disc.addr.val,6)==0;
#else
 return true;
#endif
}
static int gap_event(struct ble_gap_event*,void*);
static int discover(uint16_t c,const ble_gatt_error *err,const ble_gatt_chr *chr,void*){
 static const ble_uuid16_t target=BLE_UUID16_INIT(0xee01);
 if(err->status==0 && ble_uuid_cmp(&chr->uuid.u,&target.u)==0){
  if(!(chr->properties & BLE_GATT_CHR_PROP_WRITE_NO_RSP)){ESP_LOGW(TAG,"EE01 has no write-without-response; unsupported protocol");return 0;}
  write_handle=chr->val_handle;
  ESP_LOGI(TAG,"LED ready conn=%u EE01=%u props=0x%x",c,chr->val_handle,chr->properties);
 }else if(err->status!=0){
  ESP_LOGI(TAG,"Discovery complete status=%d ready=%d",err->status,write_handle.load()!=0);
  if(!write_handle.load())ble_gap_terminate(c,BLE_ERR_REM_USER_CONN_TERM);
 }
 return 0;
}
static int gap_event(ble_gap_event *e,void*){
 switch(e->type){
 case BLE_GAP_EVENT_DISC:{
  ble_hs_adv_fields fields={};if(ble_hs_adv_parse_fields(&fields,e->disc.data,e->disc.length_data)!=0||!exact_target(e,fields))break;
  if(connecting.exchange(true))break;
  ESP_LOGI(TAG,"BJ_LED %02x:%02x:%02x:%02x:%02x:%02x RSSI=%d",e->disc.addr.val[5],e->disc.addr.val[4],e->disc.addr.val[3],e->disc.addr.val[2],e->disc.addr.val[1],e->disc.addr.val[0],e->disc.rssi);
  ble_gap_disc_cancel();int rc=ble_gap_connect(own_type,&e->disc.addr,10000,nullptr,gap_event,nullptr);
  if(rc){connecting=false;next_attempt=esp_timer_get_time()+5000000;ESP_LOGW(TAG,"connect rc=%d",rc);}break;}
 case BLE_GAP_EVENT_CONNECT:
  connecting=false;
  if(!e->connect.status){connection=e->connect.conn_handle;ESP_LOGI(TAG,"Connected to LED");int rc=ble_gattc_disc_all_chrs(connection.load(),1,65535,discover,nullptr);if(rc)ble_gap_terminate(connection.load(),BLE_ERR_REM_USER_CONN_TERM);}
  else{next_attempt=esp_timer_get_time()+5000000;ESP_LOGW(TAG,"Connect failed status=%d",e->connect.status);}break;
 case BLE_GAP_EVENT_DISCONNECT:
  connection=BLE_HS_CONN_HANDLE_NONE;write_handle=0;connecting=false;next_attempt=esp_timer_get_time()+3000000;
  ESP_LOGW(TAG,"LED disconnected reason=%d; retry in 3s",e->disconnect.reason);break;
 default:break;
 }
 return 0;
}
static esp_err_t attr_changed(attribute::callback_type_t type,uint16_t ep,uint32_t cluster_id,uint32_t attribute_id,esp_matter_attr_val_t *value,void*){
 if(type!=attribute::POST_UPDATE||ep!=endpoint_id)return ESP_OK;
 bool changed=true;
 portENTER_CRITICAL(&state_lock);
 if(cluster_id==OnOff::Id&&attribute_id==OnOff::Attributes::OnOff::Id)desired.on=value->val.b;
 else if(cluster_id==LevelControl::Id&&attribute_id==LevelControl::Attributes::CurrentLevel::Id){desired.level=std::min<uint8_t>(value->val.u8,254);}
 else if(cluster_id==ColorControl::Id){
  if(attribute_id==ColorControl::Attributes::CurrentHue::Id)desired.h=value->val.u8;
  else if(attribute_id==ColorControl::Attributes::CurrentSaturation::Id)desired.s=value->val.u8;
  else if(attribute_id==ColorControl::Attributes::CurrentX::Id)desired.x=value->val.u16;
  else if(attribute_id==ColorControl::Attributes::CurrentY::Id)desired.y=value->val.u16;
  else if(attribute_id==ColorControl::Attributes::ColorTemperatureMireds::Id)desired.mired=value->val.u16;
  else if(attribute_id==ColorControl::Attributes::ColorMode::Id)desired.mode=value->val.u8;
  else changed=false;
 }else changed=false;
 bool exact_was_active=false;
 if(changed){
  // A real Matter update always wins over a temporary console test. The
  // override itself never enters `desired` or NVS.
  exact_was_active=exact_override.supersede_by_matter();
  ++revision;
  ++persistent_revision;
 }
 const Desired diagnostic=desired;
 const uint32_t diagnostic_revision=revision;
 portEXIT_CRITICAL(&state_lock);
 // The next deliberate picker gesture can distinguish HSV/XY/CCT and paired
 // attribute updates. Values are only light state; never log Wi-Fi or fabric data.
 if(changed && cluster_id==ColorControl::Id)
  ESP_LOGI(TAG,"Color attr rev=%lu id=0x%08lx mode=%u h=%u s=%u x=%u y=%u mired=%u level=%u",
   (unsigned long)diagnostic_revision,(unsigned long)attribute_id,diagnostic.mode,
   diagnostic.h,diagnostic.s,diagnostic.x,diagnostic.y,diagnostic.mired,diagnostic.level);
 if(exact_was_active)
  ESP_LOGI(TAG,"Exact RGB override released by Matter attribute update");
 return ESP_OK;
}
static esp_err_t identify(identification::callback_type_t,uint16_t,uint8_t,uint8_t,void*){ESP_LOGI(TAG,"Identify requested (no flashing effects)");return ESP_OK;}
static bool send_packet(const uint8_t *data,size_t size){
 int rc=ble_gattc_write_no_rsp_flat(connection.load(),write_handle.load(),data,size);
 if(rc){++failed_writes;ESP_LOGW(TAG,"BLE write rejected rc=%d",rc);return false;}
 ++successful_writes;return true;
}
static void save_desired(const Desired &d){
 nvs_handle_t n;if(nvs_open("bj_state",NVS_READWRITE,&n)!=ESP_OK)return;
 nvs_set_blob(n,"desired",&d,sizeof(d));nvs_commit(n);nvs_close(n);
}
static void worker(void*){
 uint32_t applied=0,last_saved=0;uint16_t last_connection=BLE_HS_CONN_HANDLE_NONE;int64_t last_save_time=0,last_status=0;
 for(;;){
  const auto now=esp_timer_get_time();
  if(ble_hs_synced() && connection.load()==BLE_HS_CONN_HANDLE_NONE&&!connecting.load()&&!ble_gap_disc_active()&&now>=next_attempt){
   ble_hs_id_infer_auto(0,&own_type);ble_gap_disc_params p={};p.itvl=160;p.window=48;p.filter_duplicates=1;
   int rc=ble_gap_disc(own_type,15000,&p,gap_event,nullptr);next_attempt=now+16000000;
   if(rc)ESP_LOGW(TAG,"scan rc=%d",rc);
  }
  Desired d;ExactRgbOverride exact;uint32_t rev,persist_rev;
  portENTER_CRITICAL(&state_lock);d=desired;exact=exact_override;rev=revision;persist_rev=persistent_revision;portEXIT_CRITICAL(&state_lock);
  auto c=connection.load();if(c!=last_connection){applied=0;last_connection=c;}
  if(write_handle.load()&&c!=BLE_HS_CONN_HANDLE_NONE&&rev&&rev!=applied){
   bool ok=false;
   if(exact.active){
    // Console RGB tests bypass conversion and calibration, but still use this
    // sole BLE worker and its normal connection/error handling.
    const RGB rgb=exact.rgb;
    ESP_LOGI(TAG,"Exact RGB requested rgb=%u,%u,%u (temporary; optimistic write follows)",rgb.r,rgb.g,rgb.b);
    const auto color=bj_led_protocol::rgb(rgb.r,rgb.g,rgb.b);
    ok=send_packet(color.data(),color.size());vTaskDelay(pdMS_TO_TICKS(80));
    if(ok)ok=send_packet(bj_led_protocol::kOn.data(),bj_led_protocol::kOn.size());
   }else if(d.on&&d.level){RGB rgb=d.mode==0?hsv_rgb(d.h,d.s,d.level):(d.mode==2?kelvin_rgb(d.mired,d.level):xy_rgb(d.x,d.y,d.level));
    rgb=calibrated_rgb(rgb,led_calibration);
    ESP_LOGI(TAG,"Color output rev=%lu mode=%u h=%u s=%u x=%u y=%u mired=%u level=%u rgb=%u,%u,%u",
     (unsigned long)rev,d.mode,d.h,d.s,d.x,d.y,d.mired,d.level,rgb.r,rgb.g,rgb.b);
    const auto color=bj_led_protocol::rgb(rgb.r,rgb.g,rgb.b);
    ok=send_packet(color.data(),color.size());vTaskDelay(pdMS_TO_TICKS(80));
    if(ok)ok=send_packet(bj_led_protocol::kOn.data(),bj_led_protocol::kOn.size());
   }else{ok=send_packet(bj_led_protocol::kOff.data(),bj_led_protocol::kOff.size());}
   if(ok){applied=rev;ESP_LOGI(TAG,"%s applied rev=%lu (optimistic)",exact.active?"Exact RGB":"Desired state",(unsigned long)rev);}
   else{ble_gap_terminate(c,BLE_ERR_REM_USER_CONN_TERM);}
  }
  // Do not serialize a raw test selection. Only Matter-backed desired state is saved.
  if(!exact.active&&persist_rev&&last_saved!=persist_rev&&now-last_save_time>5000000){save_desired(d);last_saved=persist_rev;last_save_time=now;}
  if(now-last_status>60000000){ESP_LOGI(TAG,"Status BLE=%s writes=%lu errors=%lu heap=%lu min=%lu",write_handle.load()?"ready":"reconnecting",(unsigned long)successful_writes.load(),(unsigned long)failed_writes.load(),(unsigned long)esp_get_free_heap_size(),(unsigned long)esp_get_minimum_free_heap_size());last_status=now;}
  vTaskDelay(pdMS_TO_TICKS(250));
 }
}
static void wifi_boot(intptr_t){
 wifi_config_t existing={};if(esp_wifi_get_config(WIFI_IF_STA,&existing)!=ESP_OK)return;
 if(existing.sta.ssid[0])return; // Preserve future credentials commissioned by iPhone.
 auto err=chip::DeviceLayer::NetworkCommissioning::ESPWiFiDriver::GetInstance().ConnectWiFiNetwork(BRIDGE_WIFI_SSID,strlen(BRIDGE_WIFI_SSID),BRIDGE_WIFI_PASS,strlen(BRIDGE_WIFI_PASS));
 ESP_LOGI(TAG,"Initial WiFi config result=%" CHIP_ERROR_FORMAT,err.Format());
}
static void events(const chip::DeviceLayer::ChipDeviceEvent *e,intptr_t){
 if(e->Type==chip::DeviceLayer::DeviceEventType::kCommissioningComplete)ESP_LOGI(TAG,"Matter commissioning complete; BLE retained");
 if(e->Type==chip::DeviceLayer::DeviceEventType::kInterfaceIpAddressChanged)ESP_LOGI(TAG,"IP connectivity changed");
}
static void update_attr(uint32_t c,uint32_t a,esp_matter_attr_val_t v){attribute::update(endpoint_id,c,a,&v);}
static void console(void*){
 char line[100];size_t pos=0;
 for(;;){int c=getchar();if(c==EOF){vTaskDelay(pdMS_TO_TICKS(20));continue;}
  if(c=='\r'||c=='\n'){line[pos]=0;pos=0;
   const TestCommand test_command=parse_test_command(line);
   if(test_command.kind==TestCommandKind::Rgb){
    portENTER_CRITICAL(&state_lock);exact_override.set(test_command.rgb);++revision;portEXIT_CRITICAL(&state_lock);
    ESP_LOGI(TAG,"Exact RGB requested rgb=%u,%u,%u; temporary override active",test_command.rgb.r,test_command.rgb.g,test_command.rgb.b);
    continue;
   }
   if(test_command.kind==TestCommandKind::Resume){
    bool was_active=false;portENTER_CRITICAL(&state_lock);was_active=exact_override.clear();if(was_active)++revision;portEXIT_CRITICAL(&state_lock);
    ESP_LOGI(TAG,"Exact RGB override %s; resuming current Matter state",was_active?"cleared":"was not active");
    continue;
   }
   if(test_command.kind==TestCommandKind::InvalidRgb){ESP_LOGW(TAG,"Invalid rgb command; use: rgb R G B (each 0..255)");continue;}
   if(!strcmp(line,"status")){ESP_LOGI(TAG,"BLE ready=%u conn=%u writes=%lu errors=%lu heap=%lu",write_handle.load()!=0,connection.load(),(unsigned long)successful_writes.load(),(unsigned long)failed_writes.load(),(unsigned long)esp_get_free_heap_size());continue;}
   if(!strcmp(line,"disconnect")){if(connection.load()!=BLE_HS_CONN_HANDLE_NONE)ble_gap_terminate(connection.load(),BLE_ERR_REM_USER_CONN_TERM);continue;}
   if(!strcmp(line,"reboot")){esp_restart();continue;}
   chip::DeviceLayer::StackLock lock;
   if(!strcmp(line,"off")||!strcmp(line,"on"))update_attr(OnOff::Id,OnOff::Attributes::OnOff::Id,esp_matter_bool(!strcmp(line,"on")));
   else if(!strcmp(line,"dim")){
    update_attr(ColorControl::Id,ColorControl::Attributes::ColorMode::Id,esp_matter_uint8(0));
    update_attr(ColorControl::Id,ColorControl::Attributes::CurrentHue::Id,esp_matter_uint8(0));
    update_attr(ColorControl::Id,ColorControl::Attributes::CurrentSaturation::Id,esp_matter_uint8(0));
    update_attr(LevelControl::Id,LevelControl::Attributes::CurrentLevel::Id,esp_matter_nullable_uint8(3));
    update_attr(OnOff::Id,OnOff::Attributes::OnOff::Id,esp_matter_bool(true));
   }else if(!strcmp(line,"pair")){
    auto &manager=chip::Server::GetInstance().GetCommissioningWindowManager();if(!manager.IsCommissioningWindowOpen())manager.OpenBasicCommissioningWindow(chip::System::Clock::Seconds16(900));
   }else if(!strcmp(line,"factory-reset"))esp_matter::factory_reset();
  }else if(pos<sizeof(line)-1)line[pos++]=c;
 }
}
extern "C" void app_main(){
 esp_err_t err=nvs_flash_init();if(err==ESP_ERR_NVS_NO_FREE_PAGES||err==ESP_ERR_NVS_NEW_VERSION_FOUND){ESP_ERROR_CHECK(nvs_flash_erase());err=nvs_flash_init();}ESP_ERROR_CHECK(err);
 nvs_handle_t n;if(nvs_open("bj_state",NVS_READONLY,&n)==ESP_OK){size_t size=sizeof(desired);if(nvs_get_blob(n,"desired",&desired,&size)==ESP_OK&&size==sizeof(desired)){saved_state=true;revision=1;persistent_revision=1;}nvs_close(n);}
 node::config_t nc;node_t *node=node::create(&nc,attr_changed,identify);assert(node);
 endpoint::extended_color_light::config_t lc;
 lc.on_off.on_off=desired.on;lc.level_control.current_level=desired.level;lc.level_control.on_level=desired.level;
 lc.color_control.color_mode=desired.mode;lc.color_control.enhanced_color_mode=desired.mode;
 lc.color_control_xy.current_x=desired.x;lc.color_control_xy.current_y=desired.y;
 lc.color_control_color_temperature.color_temperature_mireds=desired.mired;
 auto *ep=endpoint::extended_color_light::create(node,&lc,ENDPOINT_FLAG_NONE,nullptr);assert(ep);endpoint_id=endpoint::get_id(ep);
 auto *color=cluster::get(endpoint_id,ColorControl::Id);cluster::color_control::feature::hue_saturation::config_t hue;hue.current_hue=desired.h;hue.current_saturation=desired.s;cluster::color_control::feature::hue_saturation::add(color,&hue);
 esp_matter::set_custom_commissionable_data_provider(&pairing);
 ESP_ERROR_CHECK(esp_matter::start(events));
 chip::DeviceLayer::PlatformMgr().ScheduleWork(wifi_boot,0);
 xTaskCreate(worker,"bj_ble",6144,nullptr,5,nullptr);xTaskCreate(console,"bj_console",6144,nullptr,4,nullptr);
 ESP_LOGI(TAG,"Bridge started endpoint=%u; no initial LED write unless previous state was saved",endpoint_id);
}
