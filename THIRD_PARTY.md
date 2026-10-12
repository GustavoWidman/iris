# attribution and license scope

## what's included

original iris application, test/tooling code, documentation and SVG artwork are
provided under the root [MIT license](LICENSE), with the scoped exceptions below. no vendor SDK source, character
artwork, brand logos or firmware binaries are included.

`sdkconfig.defaults` is based on Espressif ESP-Matter example configuration
presets. it is conservatively identified as **Apache-2.0**, with the license in
[licenses/Apache-2.0.txt](licenses/Apache-2.0.txt); iris-specific configuration
changes do not replace that upstream license. ESP-Matter's light example initialization is marked Public Domain / CC0
upstream; no exclusive authorship of that initialization is claimed here.
the dependency lock records
package resolution, not ownership or a relicensing grant.

## protocol research

the BJ_LED wire frames and characteristic were researched using:

- Will Cooke / 8none1's [8none1/bj_led](https://github.com/8none1/bj_led), especially its
  [implementation](https://github.com/8none1/bj_led/blob/main/custom_components/bj_led/bjled.py)
  and [GATT captures](https://github.com/8none1/bj_led/blob/main/bt_snoops/gatt_bytes.txt).
- [muntahics/ELK-BLEDOM](https://github.com/muntahics/ELK-BLEDOM), related protocol
  notes (not a compatibility guarantee).
- [MohuanLED FAQ](https://mohuanled.com/frequently-asked-questions/), device names.

these are factual protocol references. their Python implementations and captured
logs are not vendored, and their licenses are not changed by iris's MIT license.
the current bridge frames are deliberately tested in `main/bj_led_protocol.h`.

## color math

the D65 XYZ-to-sRGB matrix and transfer function follow the standard conversion
also described in [W3C CSS Color 4](https://www.w3.org/TR/css-color-4/#color-conversion-code).
the compact Kelvin approximation uses the coefficients described by
[Tanner Helland](https://tannerhelland.com/2012/09/18/convert-temperature-rgb-algorithm-code.html),
whose fit credits Mitchell Charity's blackbody data. the derived `kelvin_rgb` C++ port retains Tanner Helland's **BSD-2-Clause**
notice (Copyright © 2018 Tanner Helland) in
[licenses/Tanner-Helland-BSD-2-Clause.txt](licenses/Tanner-Helland-BSD-2-Clause.txt).
this scoped notice applies to that function, not all of `color_math.h`.
it is not a scientific or calibrated color-temperature conversion, especially
outside the approximation's recommended 1000–40000 K range.

## fetched separately

| Dependency | Source / license |
| --- | --- |
| ESP-IDF | [espressif/esp-idf](https://github.com/espressif/esp-idf), primarily Apache-2.0; individual components have their own notices |
| ESP-Matter | [espressif/esp-matter](https://github.com/espressif/esp-matter), Apache-2.0 |
| Matter / connectedhomeip | [project-chip/connectedhomeip](https://github.com/project-chip/connectedhomeip), Apache-2.0 with third-party notices |
| NimBLE | [Apache Mynewt NimBLE](https://github.com/apache/mynewt-nimble), Apache-2.0; fetched through the SDK |
| pyserial (optional serial tool) | [pyserial](https://github.com/pyserial/pyserial), BSD-3-Clause |
| Managed ESP components | Resolved in `dependencies.lock`; inspect each fetched package's license before redistribution |

the root MIT license does **not** relicense these dependencies. anyone
distributing a built firmware image must review the actual resolved SDK/component
licenses and notices in addition to protecting their credentials. this project
intentionally distributes source rather than secret-bearing firmware images.
