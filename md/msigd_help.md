```sh
./msigd --help
```

```
Usage: msigd [OPTION]... 
Query or set monitor settings by usb directly on the device.
For supported devices please refer to the documentation.

Options are processed in the order they are given. You may
specify an option more than once with identical or different
values. After processing, mystic led settings are sent first
to the device. Afterwards all setting changes are sent to
the device. Once completed query settings are queried.
The --wait option which will be executed last.

All device settings provide which operations are possible:
R: Read, W: Write, RW: Read/Write


Warning:
    Use msigd only if you are sure what you are doing.
    The monitor firmware seems to have no protection against unsupported accesses.
    Using msigd may make your monitor permantently unusable.

Options:

  -q, --query                display all monitor settings. This will also
                               list readonly settings.
  -l, --list                 list all available monitors.
                               Obtains a comma separated list of all
                               MSI monitors connected. The first element
                               in the list is the monitor number to be used
                               as the argument to the --monitor option
  -m, --monitor              logical monitor number.
                               The argument to this option is the monitor
                               number as provided by the --list option
                               If omitted, the first monitor found is used
  -s, --serial               serial number of the monitor to control.
                               Use the serial number to identify the target
                               monitor in a multi-monitor environment
                               If omitted and --monitor is omitted as well
                               the first monitor found is used
      --info                 display device information. This can be used
                               with --query
  -f, --filter               limits query result to comma separated list
                               of settings, e.g. -f contrast,gamma
  -w, --wait                 SETTING=VALUE. Wait for SETTING to become
                               VALUE, e.g. macro_key=pressed
  -n, --numeric              monitor settings are displayed as numeric
                               settings
       --mystic              Expects ledgroup:mode:colors
                               ledgroup is 0 or 1, colors is a comma
                               separated list of colors to be written to
                               leds in ledgroup. The last element is used
                               to set remaining leds. mode is one of:
                               off, static, breathing, blinking, flashing,
                               blinds, meteor, rainbow, random,
                               Color format is 0xRRGGBB
                               Only available on supported monitors.

Multi monitor support:
    Use --list to get a list of all attached monitors.
    If the serial numbers provided by this list are unique,
    you can use the serial numbers to identify monitors using
    the --serial option. If you have multiple monitors of the
    same type this is most likely not the case. This is an MSI
    issue. In this case use the --monitor option to specify the
    logical monitor number provided by the --list option


All monitors:
    These options apply to all monitors:

      --macro_key             R values: off pressed 
      --serial                R values: 0 to 100
      --frequency             R values: 0 to 100
      --response_time        RW values: normal fast fastest 
      --eye_saver            RW values: off on 
      --image_enhancement    RW values: off weak medium strong strongest 
      --brightness           RW values: 0 to 100
      --contrast             RW values: 0 to 100
      --sharpness            RW values: 0 to 5
      --color_rgb            RW tripple: v1,v2,v3 where v<=100
      --osd_transparency     RW values: 0 to 5
      --osd_timeout          RW values: 0 to 30

MAG series monitors:
    These options apply to all MAG monitors:

      --enable_dynamic       RW values: on off 
      --hdcr                 RW values: off on 
      --refresh_display      RW values: off on 
      --refresh_position     RW values: left_top right_top left_bottom right_bottom 
      --alarm_clock          RW values: off 1 2 3 4 
      --alarm_position       RW values: left_top right_top left_bottom right_bottom 
      --screen_assistance    RW values: off red1 red2 red3 red4 red5 red6 white1 white2 white3 white4 white5 white6 
      --color_preset         RW values: cool normal warm custom 
      --color_red            RW values: 0 to 100
      --color_green          RW values: 0 to 100
      --color_blue           RW values: 0 to 100
      --osd_language          R values: 0 to 19
      --sound_enable         RW values: off on 

MPG series monitors:
    These options apply to all MPG monitors:

      --game_mode            RW values: user fps racing rts rpg 
      --hdcr                 RW values: off on 
      --refresh_display      RW values: off on 
      --refresh_position     RW values: left_top right_top left_bottom right_bottom 
      --free_sync            RW values: off on 
      --color_preset         RW values: cool normal warm custom 
      --color_red            RW values: 0 to 100
      --color_green          RW values: 0 to 100
      --color_blue           RW values: 0 to 100

Unknown Series:
    These options apply to the Unknown Series:


MAG321CURV:
    These options apply to the MAG321CURV:

      --power                 W values: off 
      --mode                 RW values: user fps racing rts rpg mode5 mode6 mode7 mode8 mode9 user reader cinema designer 
      --alarm_clock_index    RW values: 1 to 4
      --alarm_clock_time     RW values: 0 to 5999
      --alarm4x               W a1,a2,a3,a4,n where a<5999 and 1=<n<=4
      --zero_latency         RW values: off on 
      --screen_size          RW values: 19 24 4:3 16:9 
      --night_vision         RW values: off normal strong strongest ai 
      --pro_mode             RW values: user reader cinema designer 
      --input                RW values: hdmi1 hdmi2 dp usbc 
      --pip                  RW values: off pip pbp 
      --pip_input            RW values: hdmi1 hdmi2 dp usbc 
      --pbp_input            RW values: hdmi1 hdmi2 dp usbc 
      --pip_size             RW values: small medium large 
      --pip_position         RW values: left_top right_top left_bottom right_bottom 
      --toggle_display        W values: on 
      --toggle_sound          W values: on 
      --rgb_led              RW values: off on 
      --navi_up              RW values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 
      --navi_down            RW values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 
      --navi_left            RW values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 
      --navi_right           RW values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 

MAG321CQR:
    These options apply to the MAG321CQR:

      --mode                 RW values: user fps racing rts rpg mode5 mode6 mode7 mode8 mode9 user reader cinema designer 
      --zero_latency         RW values: off on 
      --screen_size          RW values: 19 24 4:3 16:9 
      --pro_mode             RW values: user reader cinema designer 
      --input                RW values: hdmi1 hdmi2 dp 
      --pip                  RW values: off pip pbp 
      --pip_input            RW values: hdmi1 hdmi2 dp 
      --pbp_input            RW values: hdmi1 hdmi2 dp 
      --pip_size             RW values: small medium large 
      --pip_position         RW values: left_top right_top left_bottom right_bottom 
      --toggle_display        W values: on 
      --toggle_sound          W values: on 
      --rgb_led              RW values: off on 
      --navi_up              RW values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 
      --navi_down            RW values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 
      --navi_left            RW values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 
      --navi_right           RW values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 

MAG321QR:
    These options apply to the MAG321QR:

      --mode                 RW values: user fps racing rts rpg mode5 mode6 mode7 mode8 mode9 user anti_blue movie office srgb eco 
      --alarm_clock_time     RW values: 0 to 5999
      --alarm_position       RW values: left_top right_top left_bottom right_bottom custom 
      --smart_crosshair_icon RW values: off icon1 icon2 icon3 icon4 icon5 icon6 
      --smart_crosshair_color RW values: white red auto 
      --screen_size          RW values: auto 4:3 16:9 
      --night_vision         RW values: off normal strong strongest ai 
      --pro_mode             RW values: user anti_blue movie office srgb eco 
      --input                RW values: hdmi1 hdmi2 dp usbc 
      --auto_scan            RW values: off on 
      --screen_info          RW values: off on 
      --rgb_led              RW values: off on 
      --power_button         RW values: off standby 
      --hdmi_cec             RW values: off on 
      --kvm                  RW values: auto upstream type_c 
      --navi_up              RW values: off brightness game_mode alarm_clock smart_crosshair input refresh_rate info night_vision kvm 
      --navi_down            RW values: off brightness game_mode alarm_clock smart_crosshair input refresh_rate info night_vision kvm 
      --navi_left            RW values: off brightness game_mode alarm_clock smart_crosshair input refresh_rate info night_vision kvm 
      --navi_right           RW values: off brightness game_mode alarm_clock smart_crosshair input refresh_rate info night_vision kvm 

MAG241C:
    These options apply to the MAG241C:

      --black_tuner          RW values: 0 to 20
      --pro_mode             RW values: user reader cinema designer 
      --input                RW values: hdmi1 hdmi2 dp 
      --navi_up              RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate 
      --navi_down            RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate 
      --navi_left            RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate 
      --navi_right           RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate 

MAG241C:
    These options apply to the MAG241C:

      --black_tuner          RW values: 0 to 20
      --pro_mode             RW values: user reader cinema designer 
      --input                RW values: hdmi1 hdmi2 dp 
      --navi_up              RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate 
      --navi_down            RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate 
      --navi_left            RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate 
      --navi_right           RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate 

MAG241CR:
    These options apply to the MAG241CR:

      --black_tuner          RW values: 0 to 20
      --pro_mode             RW values: user reader cinema designer 
      --input                RW values: hdmi1 hdmi2 dp 
      --rgb_led              RW values: off on 
      --navi_up              RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate 
      --navi_down            RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate 
      --navi_left            RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate 
      --navi_right           RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate 

MAG271CR:
    These options apply to the MAG271CR:

      --black_tuner          RW values: 0 to 20
      --pro_mode             RW values: user reader cinema designer 
      --input                RW values: hdmi1 hdmi2 dp 
      --rgb_led              RW values: off on 
      --navi_up              RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate 
      --navi_down            RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate 
      --navi_left            RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate 
      --navi_right           RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate 

MAG271CQR:
    These options apply to the MAG271CQR:

      --black_tuner          RW values: 0 to 20
      --zero_latency         RW values: off on 
      --screen_size          RW values: 19 24 4:3 16:9 
      --pro_mode             RW values: user reader cinema designer 
      --input                RW values: hdmi1 hdmi2 dp 
      --pip                  RW values: off pip pbp 
      --pip_input            RW values: hdmi1 hdmi2 dp 
      --pbp_input            RW values: hdmi1 hdmi2 dp 
      --pip_size             RW values: small medium large 
      --pip_position         RW values: left_top right_top left_bottom right_bottom 
      --toggle_display        W values: on 
      --toggle_sound          W values: on 
      --rgb_led              RW values: off on 
      --navi_up              RW values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 
      --navi_down            RW values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 
      --navi_left            RW values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 
      --navi_right           RW values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 

MAG272CQR:
    These options apply to the MAG272CQR:

      --mode                 RW values: user fps racing rts rpg mode5 mode6 mode7 mode8 mode9 user reader cinema designer HDR 
      --zero_latency         RW values: off on 
      --screen_size          RW values: auto 4:3 16:9 
      --night_vision         RW values: off normal strong strongest ai 
      --pro_mode             RW values: user reader cinema designer HDR 
      --input                RW values: hdmi1 hdmi2 dp usbc 
      --rgb_led              RW values: off on 
      --navi_up              RW values: off brightness game_mode screen_assistance alarm_clock refresh_rate info 
      --navi_down            RW values: off brightness game_mode screen_assistance alarm_clock refresh_rate info 
      --navi_left            RW values: off brightness game_mode screen_assistance alarm_clock refresh_rate info 
      --navi_right           RW values: off brightness game_mode screen_assistance alarm_clock refresh_rate info 

MAG272QR:
    These options apply to the MAG272QR:

      --mode                 RW values: user fps racing rts rpg mode5 mode6 mode7 mode8 mode9 user reader cinema designer HDR 
      --zero_latency         RW values: off on 
      --screen_size          RW values: auto 4:3 16:9 
      --night_vision         RW values: off normal strong strongest ai 
      --pro_mode             RW values: user reader cinema designer HDR 
      --input                RW values: hdmi1 hdmi2 dp usbc 
      --rgb_led              RW values: off on 
      --navi_up              RW values: off brightness game_mode screen_assistance alarm_clock refresh_rate info 
      --navi_down            RW values: off brightness game_mode screen_assistance alarm_clock refresh_rate info 
      --navi_left            RW values: off brightness game_mode screen_assistance alarm_clock refresh_rate info 
      --navi_right           RW values: off brightness game_mode screen_assistance alarm_clock refresh_rate info 

MAG272:
    These options apply to the MAG272:

      --mode                 RW values: user fps racing rts rpg mode5 mode6 mode7 mode8 mode9 user reader cinema designer HDR 
      --zero_latency         RW values: off on 
      --screen_size          RW values: auto 4:3 16:9 
      --night_vision         RW values: off normal strong strongest ai 
      --pro_mode             RW values: user reader cinema designer HDR 
      --input                RW values: hdmi1 hdmi2 dp usbc 
      --navi_up              RW values: off brightness game_mode screen_assistance alarm_clock refresh_rate info 
      --navi_down            RW values: off brightness game_mode screen_assistance alarm_clock refresh_rate info 
      --navi_left            RW values: off brightness game_mode screen_assistance alarm_clock refresh_rate info 
      --navi_right           RW values: off brightness game_mode screen_assistance alarm_clock refresh_rate info 

MAG272QP:
    These options apply to the MAG272QP:

      --mode                 RW values: user fps racing rts rpg mode5 mode6 mode7 mode8 mode9 user reader cinema designer HDR 
      --zero_latency         RW values: off on 
      --screen_size          RW values: auto 4:3 16:9 
      --night_vision         RW values: off normal strong strongest ai 
      --pro_mode             RW values: user reader cinema designer HDR 
      --input                RW values: hdmi1 hdmi2 dp usbc 
      --navi_up              RW values: off brightness game_mode screen_assistance alarm_clock refresh_rate info 
      --navi_down            RW values: off brightness game_mode screen_assistance alarm_clock refresh_rate info 
      --navi_left            RW values: off brightness game_mode screen_assistance alarm_clock refresh_rate info 
      --navi_right           RW values: off brightness game_mode screen_assistance alarm_clock refresh_rate info 

MPG27CQ:
    These options apply to the MPG27CQ:

      --black_tuner          RW values: 0 to 20
      --zero_latency         RW values: off on 
      --screen_size          RW values: 19 24 4:3 16:9 
      --pro_mode             RW values: user reader cinema designer 
      --input                RW values: hdmi1 hdmi2 dp 
      --pip                  RW values: off pip pbp 
      --pip_input            RW values: hdmi1 hdmi2 dp 
      --pbp_input            RW values: hdmi1 hdmi2 dp 
      --pip_size             RW values: small medium large 
      --pip_position         RW values: left_top right_top left_bottom right_bottom 
      --toggle_display        W values: on 
      --toggle_sound          W values: on 
      --audio_source         RW values: analog digital 
      --navi_up              RW values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 
      --navi_down            RW values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 
      --navi_left            RW values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 
      --navi_right           RW values: off brightness game_mode screen_assistance alarm_clock input pip refresh_rate 

MPG273CQR:
    These options apply to the MPG273CQR:

      --mode                 RW values: user fps racing rts rpg mode5 mode6 mode7 mode8 mode9 user anti_blue movie office srgb eco 
      --alarm_clock_time     RW values: 0 to 5999
      --alarm_position       RW values: left_top right_top left_bottom right_bottom custom 
      --smart_crosshair_icon RW values: off icon1 icon2 icon3 icon4 icon5 icon6 
      --smart_crosshair_color RW values: white red auto 
      --screen_size          RW values: auto 4:3 16:9 
      --night_vision         RW values: off normal strong strongest ai 
      --pro_mode             RW values: user anti_blue movie office srgb eco 
      --input                RW values: hdmi1 hdmi2 dp usbc 
      --auto_scan            RW values: off on 
      --screen_info          RW values: off on 
      --rgb_led              RW values: off on 
      --power_button         RW values: off standby 
      --hdmi_cec             RW values: off on 
      --ambient_brightness   RW values: off auto custom 
      --ambient_rgb          RW values: off on 
      --ambient_brightness_custom RW values: 0 to 100
      --kvm                  RW values: auto upstream type_c 
      --sound_tune           RW values: off on 
      --navi_up              RW values: off brightness game_mode smart_crosshair alarm_clock input refresh_rate info night_vision kvm 
      --navi_down            RW values: off brightness game_mode smart_crosshair alarm_clock input refresh_rate info night_vision kvm 
      --navi_left            RW values: off brightness game_mode smart_crosshair alarm_clock input refresh_rate info night_vision kvm 
      --navi_right           RW values: off brightness game_mode smart_crosshair alarm_clock input refresh_rate info night_vision kvm 

MPG341CQR:
    These options apply to the MPG341CQR:

      --zero_latency         RW values: off on 
      --screen_size          RW values: auto 4:3 16:9 21:9 1:1 
      --night_vision         RW values: off normal strong strongest ai 
      --pro_mode             RW values: user reader cinema designer 
      --input                RW values: hdmi1 hdmi2 dp usbc 
      --pip                  RW values: off pip pbp_x2 pbp_x3 pbp_x4 
      --pip_input            RW values: hdmi1 hdmi2 dp usbc 
      --pip_size             RW values: small medium large 
      --pip_position         RW values: left_top right_top left_bottom right_bottom 
      --toggle_display        W values: on 
      --pip_sound_source     RW values: hdmi1 hdmi2 dp usbc 
      --pbp_input1           RW values: hdmi1 hdmi2 dp usbc 
      --pbp_input2           RW values: hdmi1 hdmi2 dp usbc 
      --pbp_input3           RW values: hdmi1 hdmi2 dp usbc 
      --pbp_sound_source     RW values: hdmi1 hdmi2 dp usbc 
      --audio_source         RW values: analog digital 
      --rgb_led              RW values: off on 
      --navi_up              RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate audio_volume 
      --navi_down            RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate audio_volume 
      --navi_left            RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate audio_volume 
      --navi_right           RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate audio_volume 

MAG274R:
    These options apply to the MAG274R:

      --screen_size          RW values: auto 4:3 16:9 
      --night_vision         RW values: off normal strong strongest ai 
      --pro_mode             RW values: user reader cinema designer 
      --input                RW values: hdmi1 hdmi2 dp usbc 
      --auto_scan            RW values: off on 
      --screen_info          RW values: off on 
      --rgb_led              RW values: off on 
      --power_button         RW values: off standby 
      --navi_up              RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate info 
      --navi_down            RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate info 
      --navi_left            RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate info 
      --navi_right           RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate info 

MAG251RX:
    These options apply to the MAG251RX:

      --alarm_position       RW values: left_top right_top left_bottom right_bottom custom 
      --screen_assistance     R values: off icon1 icon2 icon3 icon4 icon5 icon6 
      --screen_size          RW values: auto 4:3 16:9 
      --night_vision         RW values: off normal strong strongest ai 
      --pro_mode             RW values: user reader cinema designer HDR 
      --input                RW values: hdmi1 hdmi2 dp usbc 
      --screen_info          RW values: off on 
      --rgb_led              RW values: off on 
      --power_button         RW values: off standby 
      --navi_up              RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate info 
      --navi_down            RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate info 
      --navi_left            RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate info 
      --navi_right           RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate info 

MAG274QRF-QD FW.011:
    These options apply to the MAG274QRF-QD FW.011:

      --mode                 RW values: user fps racing rts rpg mode5 mode6 mode7 mode8 mode9 user reader cinema office 
      --night_vision         RW values: off normal strong strongest ai 
      --pro_mode             RW values: user reader cinema office 
      --input                RW values: hdmi1 hdmi2 dp usbc 
      --auto_scan            RW values: off on 
      --screen_info          RW values: off on 
      --rgb_led              RW values: off on 
      --power_button         RW values: off standby 
      --hdmi_cec             RW values: off on 
      --navi_up              RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate info 
      --navi_down            RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate info 
      --navi_left            RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate info 
      --navi_right           RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate info 

MAG274QRF-QD FW.015/FW.016:
    These options apply to the MAG274QRF-QD FW.015/FW.016:

      --mode                 RW values: user fps racing rts rpg mode5 mode6 mode7 mode8 mode9 user reader cinema office srgb adobe_rgb dci_p3 
      --night_vision         RW values: off normal strong strongest ai 
      --pro_mode             RW values: user reader cinema office srgb adobe_rgb dci_p3 
      --input                RW values: hdmi1 hdmi2 dp usbc 
      --auto_scan            RW values: off on 
      --screen_info          RW values: off on 
      --rgb_led              RW values: off on 
      --power_button         RW values: off standby 
      --hdmi_cec             RW values: off on 
      --navi_up              RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate info 
      --navi_down            RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate info 
      --navi_left            RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate info 
      --navi_right           RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate info 

MAG274QRF-QD FW.020:
    These options apply to the MAG274QRF-QD FW.020:

      --mode                 RW values: user fps racing rts rpg premium_color mode6 mode7 mode8 mode9 user reader cinema office srgb adobe_rgb dci_p3 
      --game_mode            RW values: user fps racing rts rpg premium_color 
      --night_vision         RW values: off normal strong strongest ai 
      --pro_mode             RW values: user reader cinema office srgb adobe_rgb dci_p3 
      --input                RW values: hdmi1 hdmi2 dp usbc 
      --auto_scan            RW values: off on 
      --screen_info          RW values: off on 
      --rgb_led              RW values: off on 
      --power_button         RW values: off standby 
      --hdmi_cec             RW values: off on 
      --navi_up              RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate info 
      --navi_down            RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate info 
      --navi_left            RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate info 
      --navi_right           RW values: off brightness game_mode screen_assistance alarm_clock input refresh_rate info 

PS341WU:
    These options apply to the PS341WU:

      --mode                 RW values: user adobe_rgb dci_p3 srgb hdr cinema reader bw dicom eyecare cal1 cal2 cal3 
      --quick_charge          R values: off on 
      --alarm_position       RW values: left_top right_top left_bottom right_bottom custom 
      --screen_assistance    RW values: off center edge scale_v scale_h line_v line_h grid thirds 3D_assistance 
      --screen_size          RW values: auto 4:3 16:9 21:9 1:1 
      --pro_mode             RW values: user adobe_rgb dci_p3 srgb hdr cinema reader bw dicom eyecare cal1 cal2 cal3 
      --color_preset         RW values: 5000K 5500K 6500K 7500K 9300K 10000K custom 
      --gray_level           RW values: 0 to 20
      --low_blue_light       RW values: off on 
      --local_dimming        RW values: off on 
      --hue_rgb              RW tripple: v1,v2,v3 where v<=100
      --hue_cmy              RW tripple: v1,v2,v3 where v<=100
      --zoom                 RW values: off on 
      --zoom_location        RW values: center left_top right_top left_bottom right_bottom 
      --saturation_rgb       RW tripple: v1,v2,v3 where v<=100
      --saturation_cmy       RW tripple: v1,v2,v3 where v<=100
      --gamma                RW values: 1.8 2 2.2 2.4 2.6 
      --input                RW values: hdmi1 hdmi2 dp usbc 
      --pip                  RW values: off pip pbp_x2 pbp_x3 pbp_x4 
      --pip_input            RW values: hdmi1 hdmi2 dp usbc 
      --pip_size             RW values: small medium large 
      --pip_position         RW values: left_top right_top left_bottom right_bottom 
      --toggle_display        W values: on 
      --pip_sound_source     RW values: hdmi1 hdmi2 dp usbc 
      --pbp_input1           RW values: hdmi1 hdmi2 dp usbc 
      --pbp_input2           RW values: hdmi1 hdmi2 dp usbc 
      --pbp_input3           RW values: hdmi1 hdmi2 dp usbc 
      --pbp_input4           RW values: hdmi1 hdmi2 dp usbc 
      --pbp_sound_source     RW values: hdmi1 hdmi2 dp usbc 
      --osd_language          R values: 0 to 28
      --screen_info          RW values: off on 
      --audio_source         RW values: analog digital 
      --navi_up              RW values: off brightness pro_mode screen_assistance alarm_clock input pip zoom_in info 
      --navi_down            RW values: off brightness pro_mode screen_assistance alarm_clock input pip zoom_in info 
      --navi_left            RW values: off brightness pro_mode screen_assistance alarm_clock input pip zoom_in info 
      --navi_right           RW values: off brightness pro_mode screen_assistance alarm_clock input pip zoom_in info 

General options:
    These options always apply:

  -d, --debug                enable debug output
                               Enables raw output for query command
  -h, --help                 display this help and exit
      --version              output version information and exit

Exit status:
 0  if OK,
 1  if error during option parsing,
 2  if error during device identification,
 3  if error during setting parameters on device,
 4  if error during reading parameters from device,

Report bugs on <https://github.com/couriersud/msigd/issues>
msigd home page: <https://github.com/couriersud/msigd>
```
