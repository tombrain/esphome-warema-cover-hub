```yaml
# example configuration:

warema_cover_hub:
  id: warema_cover_hub_1
  name: "warema_cover_hub_1_name"

cover:
  - platform: warema_cover_hub
    name: "Cover123"
    warema_cover_hub_id: warema_cover_hub_1
    open_duration: 26sec
    close_duration: 23sec
```