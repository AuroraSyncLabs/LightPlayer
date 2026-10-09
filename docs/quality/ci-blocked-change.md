# CI blocked a broken change (2026-10-09)

|              |                                                                                                                   |
|--------------|-------------------------------------------------------------------------------------------------------------------|
| Pull request | [#11](https://github.com/AuroraSyncLabs/LightPlayer/pull/11)                                                      |
| Failed job   | Format (clang-format), runs [37949779502](https://github.com/AuroraSyncLabs/LightPlayer/actions/runs/37949779502) |
| Problem      | 11 formatting violations in `src/light_control.c` and both example `main.c` files                                 |
| Fix          | `9d88d9f` style: apply clang-format, then the job passed                                                          |

<img width="1524" height="1070" alt="Screenshot_20261009_171457" src="https://github.com/user-attachments/assets/4f25667f-e950-4c39-956f-0677deedfb11" />
<img width="1698" height="1313" alt="Screenshot_20261009_171605" src="https://github.com/user-attachments/assets/d7226147-1ffd-4019-92c7-cc9f05bfcd65" />
