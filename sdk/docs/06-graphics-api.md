# Graphics API Reference

| Symbol | Args | Notes |
|--------|------|-------|
| `app_win_create` | r2=x r3=y r4=w r5=h r6=base | → window id |
| `app_win_destroy` | r2=id | |
| `app_win_fill` | r2=id r3=color | RGB packed |
| `app_win_pixel` | r2=id r3=x r4=y r5=color | |
| `app_win_raise` | r2=id | |
| `app_compose` | | compose desktop |

Framebuffer host is 64×32. Windows are owned by the creating process (M15).
