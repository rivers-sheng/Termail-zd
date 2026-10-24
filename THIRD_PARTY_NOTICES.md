# Third-Party Licenses & Notices

This project (Termail) incorporates or bundles the following third-party components. Their respective licenses apply to those components.

## 1. Core Libraries & Tools

* **Qt 6** (UI Framework)
  * **License:** LGPL-3.0 / GPL-3.0 (Dual-licensed)
  * **Source / Info:** https://www.qt.io/licensing

* **Lua 5.4** (Scripting Engine)
  * **License:** MIT License
  * **Copyright:** © 1994–2026 Lua.org, PUC-Rio.
  * **Source:** https://www.lua.org/

* **proot** (User-space chroot/chroot implementation)
  * **License:** GPL-2.0-or-later
  * **Source:** https://github.com/proot-me/proot

* **BusyBox** (Core utilities)
  * **License:** GPL-2.0 only
  * **Source:** https://busybox.net/downloads/

## 2. Linux Rootfs (Container Environments)

* **Alpine Linux Rootfs** (Bundled in current versions)
  * **Notice:** The Alpine Linux rootfs is a collection of independent open-source software packages. Each package within the rootfs is governed by its own license (e.g., BusyBox under GPL-2.0, musl libc under MIT). Termail bundles this rootfs solely as an execution environment for `proot`.
  * **Source / Info:** https://alpinelinux.org/

* **Ubuntu Rootfs** (Bundled in historical versions prior to `v6.4.0`)
  * **Notice:** The Ubuntu rootfs is an aggregate of numerous independent software works. Canonical Ltd. states that Ubuntu is an aggregate of many works, each governed by its own license. Termail bundled this rootfs in earlier releases solely as an execution environment for `proot`.
  * **Source / Info:** https://ubuntu.com/legal/intellectual-property-policy

---
*Note: If you distribute this project or any modified version of it, you must comply with the terms of the GNU General Public License v3.0 (or AGPL-3.0) for Termail's code, and the respective licenses for the third-party components listed above.*