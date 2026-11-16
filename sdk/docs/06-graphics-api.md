# Graphics API Reference

| Symbol | Args | Notes |
|--------|------|-------|
| `app_win_create` | r2=x r3=y r4=w r5=h r6=base | → window id |
| `app_win_destroy` | r2=id | |
| `app_win_fill` | r2=id r3=color | RGB packed |
| `app_win_pixel` | r2=id r3=x r4=y r5=color | |
| `app_win_raise` | r2=id | |
| `app_compose` | | compose desktop |
| `app_draw_text` | r2=id r3=x r4=y r5=fg r6=bg r7=str | `WIN_DRAW_TEXT` |
| `app_draw_char` | r2=id r3=x r4=y r5=fg r6=bg r7=char | via draw_text |
| `app_event_poll` | | → type; KEY/MOUSE fields in r2.. |

Default host framebuffer for `--gui` / desktop is **320×180** @ `0x00200000`.  
Windows are owned by the creating process. Full ABI: tree `docs/graphics-api.md`.
