# Privacy and Publication

This repository must not contain private or copyrighted test material.

Never commit:

- save files or SRAM dumps;
- ROMs or emulator binaries;
- screenshots, videos, PDFs, or private proof evidence;
- exported `.red.json` or future `.fred.json` files derived from private saves;
- local absolute paths or machine-specific helper configuration;
- generated proof/output folders or archives.

Tests must use synthetic public fixtures created for redistribution, or construct their data in memory. Public reports should use logical artifact names and omit private absolute paths. The project does not require or distribute ROMs.

The bundled Red generation template is a public synthetic/canonical engine resource documented in `THIRD_PARTY_NOTICES.md`; it is not a user save, ROM, or emulator capture. Its identity is checked at runtime.

Before every commit and release:

- inspect all untracked files as well as the ordinary diff;
- scan for credentials and private absolute paths;
- scan for prohibited save, ROM, image, document, archive, and evidence extensions;
- verify generated artifacts remain ignored;
- confirm reports and examples contain logical names only;
- confirm the two historical reference repositories remain clean.

Proof packages and emulator checklists may contain sensitive gameplay state even when no ROM is present. Keep them local unless a separate publication review establishes that every input and derived artifact is redistributable.
