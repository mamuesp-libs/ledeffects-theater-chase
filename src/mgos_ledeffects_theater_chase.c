#include "mgos.h"
#include "led_master.h"

#include "led_master.h"

typedef struct {
    int gap;
    int group;
    int pos;
    tools_rgb_data chase_color;
    tools_rgb_data color;
    tools_rgb_data out_pix;
    tools_rgb_data chase_pix;
    audio_trigger_data* atd;    
} theater_chase_data;

static theater_chase_data *main_tcd;

static theater_chase_data* mgos_intern_theater_chase_init(mgos_rgbleds* leds)
{
    int num_columns = leds->panel_width;
    
    leds->timeout = mgos_sys_config_get_ledeffects_theater_chase_timeout();
    leds->dim_all = mgos_sys_config_get_ledeffects_theater_chase_dim_all();

    theater_chase_data* result = calloc(num_columns, sizeof(theater_chase_data));
    result->atd = (audio_trigger_data*)leds->audio_data;
    
    for (int col = 0; col < num_columns; col++) {

        theater_chase_data* curr_tcd = &result[col];

        curr_tcd->pos = tools_get_random(0, num_columns - 1);
        curr_tcd->gap = mgos_sys_config_get_ledeffects_theater_chase_gap();
        curr_tcd->group = mgos_sys_config_get_ledeffects_theater_chase_group();
        curr_tcd->color = (tools_rgb_data)tools_get_random_color_fade(curr_tcd->color, &curr_tcd->color, 1, 30.0, 1.0, 0.6);
        curr_tcd->out_pix = curr_tcd->color;
        curr_tcd->chase_color = (tools_rgb_data)tools_get_random_color_fade(curr_tcd->chase_color, &curr_tcd->chase_color, 1, 30.0, 1.0, 0.6);
        curr_tcd->chase_pix = curr_tcd->chase_color;
    }
    
    mgos_universal_led_clear(leds);
    mgos_universal_led_show(leds);

    return result;
}

static void mgos_intern_theater_chase_exit(mgos_rgbleds* leds)
{
    free(main_tcd);
    main_tcd = NULL;
}

static void mgos_intern_theater_chase_loop(mgos_rgbleds* leds)
{
    uint32_t num_cols = leds->panel_width;
    uint32_t num_rows = leds->panel_height;
    tools_rgb_data curr_pix;

    for (int x = 0; x < num_cols; x++) {

        theater_chase_data* curr_tcd = &main_tcd[x];

        if (curr_tcd->pos == 0) {
            curr_tcd->color = curr_tcd->chase_color;
            curr_tcd->out_pix = curr_tcd->chase_pix;
            curr_tcd->chase_color = (tools_rgb_data)tools_get_random_color_fade(curr_tcd->chase_color, &curr_tcd->chase_color, 1, 50.0, 1.0, 0.6);
        }

        LOG(LL_VERBOSE_DEBUG, ("Show LEDs in theater chase ..."));
        //mgos_universal_led_set_all(leds, curr_tcd->out_pix);
        for (int i = 0; i < num_rows; i++) {
            int range_pos = (i % (curr_tcd->group + curr_tcd->gap));
            int y = (curr_tcd->pos + i) % num_rows;
            if (range_pos < curr_tcd->gap) {
                memset(&curr_pix, 0, sizeof(tools_rgb_data));
            } else {
                curr_pix = (y < curr_tcd->pos) ? curr_tcd->chase_pix : curr_tcd->out_pix;
            }
            mgos_universal_led_plot_pixel(leds, x, y, curr_pix, true);
        }
        curr_tcd->pos++;
        curr_tcd->pos %= num_rows;
    }
    mgos_universal_led_show(leds);
    mgos_wdt_feed();
    
}

void mgos_ledeffects_theater_chase(void* param, mgos_rgbleds_action action)
{
    static bool do_time = false;
    static uint32_t max_time = 0;
    uint32_t time = (mgos_uptime_micros() / 1000);
    mgos_rgbleds* leds = (mgos_rgbleds*)param;

    switch (action) {
    case MGOS_RGBLEDS_ACT_INIT:
        LOG(LL_INFO, ("led_master_theater_chase: called (init)"));
        main_tcd = mgos_intern_theater_chase_init(leds);
        break;
    case MGOS_RGBLEDS_ACT_EXIT:
        LOG(LL_INFO, ("led_master_theater_chase: called (exit)"));
        mgos_intern_theater_chase_exit(leds);
        break;
    case MGOS_RGBLEDS_ACT_LOOP:
        LOG(LL_VERBOSE_DEBUG, ("led_master_theater_chase: called (loop)"));
        mgos_intern_theater_chase_loop(leds);
        if (do_time) {
            time = (mgos_uptime_micros() /1000) - time;
            max_time = (time > max_time) ? time : max_time;
            LOG(LL_VERBOSE_DEBUG, ("Theater chase loop duration: %d milliseconds, max: %d ...", time / 1000, max_time / 1000));
        }
        break;
    }
}

bool mgos_ledeffects_theater_chase_init(void) {
  LOG(LL_INFO, ("mgos_theater_chase_init ..."));
  ledmaster_add_effect("ANIM_THEATER_CHASE", mgos_ledeffects_theater_chase);
  return true;
}
