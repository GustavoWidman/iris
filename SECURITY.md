# security

iris is experimental local firmware, not a certified or hardened Matter product.
do not use it for safety-critical lighting. BLE name/address matching is device
selection, not cryptographic authentication; nearby compatible devices can be
selected if no private address filter is configured.

## keep these private

- `.private/`, Wi-Fi credentials, Matter setup passcodes, salts and QR/manual codes
- Compiled firmware (which embeds bootstrap credentials and setup material)
- NVS/fabric data and full flash backups
- Raw serial logs, local command queues and device identifiers

the ignore rules reduce accidents; they are not a secret scanner. before
publishing, review both staged files and history. if something leaks, remove it
from public history **and rotate/recommission the affected secrets deliberately**;
deleting a file does not undo disclosure. Backups may preserve older secrets.

do not report vulnerabilities with live keys, raw dumps or unredacted logs in a
public issue. use GitHub's private vulnerability reporting feature if it is
enabled for this repository. if unavailable, open only a non-sensitive request
for a private reporting channel; do not publish exploit details or private data.
no response SLA or supported production release is promised.
