/**
 * @file ui_panels.c
 * @brief Panel contents
 *
 * Two windows: a control panel on the left with collapsible sections, and
 * an inspector on the right that appears when an ant is selected. Every
 * button routes through app_do_action(), the same path as the keyboard, so
 * the two can never disagree.
 */

#include "ui_panels.h"
#include "ui.h"
#include "app.h"
#include "utils.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define PANEL_MARGIN      12
#define LEFT_PANEL_WIDTH  340
#define RIGHT_PANEL_WIDTH 330
#define GRAPH_HEIGHT      64
#define NETWORK_HEIGHT    250

static const mu_Color CLR_ACCENT   = {100, 200, 255, 255};
static const mu_Color CLR_MUTED    = {150, 154, 168, 255};
static const mu_Color CLR_GOOD     = {110, 225, 150, 255};
static const mu_Color CLR_WARN     = {245, 180, 90, 255};
static const mu_Color CLR_BAD      = {235, 90, 90, 255};
static const mu_Color CLR_GRAPH_BG = {14, 15, 20, 220};
static const mu_Color CLR_POSITIVE = {90, 190, 255, 255};
static const mu_Color CLR_NEGATIVE = {245, 150, 80, 255};

/* ============== Small widgets ============== */

static void row_full(mu_Context *ctx) {
    int widths[1] = {-1};
    mu_layout_row(ctx, 1, widths, 0);
}

/** @brief Width available for controls inside the current panel */
static int content_width(mu_Context *ctx) {
    mu_Container *cnt = mu_get_current_container(ctx);
    return cnt->body.w - ctx->style->padding * 2;
}

/**
 * @brief Start a row of n equally wide controls
 *
 * microui reads a negative width as "extend to this far from the right
 * edge", so a row of -1s would stack every control on the same spot. Only
 * the last column gets -1; the rest are measured.
 */
static void row_split(mu_Context *ctx, int n) {
    int widths[8];
    if (n < 1) n = 1;
    if (n > 8) n = 8;

    int spacing = ctx->style->spacing;
    int each = (content_width(ctx) - spacing * (n - 1)) / n;
    for (int i = 0; i < n; i++) {
        widths[i] = each;
    }
    widths[n - 1] = -1;
    mu_layout_row(ctx, n, widths, 0);
}

/** @brief Row of a fixed-width label followed by a control filling the rest */
static void row_label_control(mu_Context *ctx, int label_width) {
    int widths[2] = {label_width, -1};
    mu_layout_row(ctx, 2, widths, 0);
}

/** @brief Label on the left, value right-aligned on the same line */
static void stat_row(mu_Context *ctx, const char *label, const char *value) {
    int widths[2] = {content_width(ctx) * 45 / 100, -1};
    mu_layout_row(ctx, 2, widths, 0);
    mu_label(ctx, label);
    mu_draw_control_text(ctx, value, mu_layout_next(ctx), MU_COLOR_TEXT, MU_OPT_ALIGNRIGHT);
}

static void stat_rowf(mu_Context *ctx, const char *label, const char *fmt, ...) {
    char buf[64];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    stat_row(ctx, label, buf);
}

static bool toggle(mu_Context *ctx, const char *label, bool *value) {
    /* The control id hashes the pointer, so a per-label id keeps the
       temporaries below from colliding with each other. */
    mu_push_id(ctx, label, (int)strlen(label));
    int state = *value ? 1 : 0;
    int res = mu_checkbox(ctx, label, &state);
    mu_pop_id(ctx);

    if (res & MU_RES_CHANGE) {
        *value = state != 0;
        return true;
    }
    return false;
}

static void slider_float(mu_Context *ctx, const char *label, float *value,
                         float low, float high, float step, const char *fmt) {
    row_label_control(ctx, content_width(ctx) * 45 / 100);
    mu_label(ctx, label);
    mu_push_id(ctx, label, (int)strlen(label));
    mu_slider_ex(ctx, value, low, high, step, fmt, MU_OPT_ALIGNCENTER);
    mu_pop_id(ctx);
}

static void slider_int(mu_Context *ctx, const char *label, int *value, int low, int high) {
    row_label_control(ctx, content_width(ctx) * 45 / 100);
    mu_label(ctx, label);

    mu_push_id(ctx, label, (int)strlen(label));
    float v = (float)*value;
    if (mu_slider_ex(ctx, &v, (float)low, (float)high, 1.0f, "%.0f", MU_OPT_ALIGNCENTER) &
        MU_RES_CHANGE) {
        *value = (int)(v + 0.5f);
    }
    mu_pop_id(ctx);
}

/** @brief Button that looks pressed while its option is the active one */
static bool option_button(mu_Context *ctx, const char *label, bool active) {
    mu_Color saved_button = ctx->style->colors[MU_COLOR_BUTTON];
    mu_Color saved_text = ctx->style->colors[MU_COLOR_TEXT];
    if (active) {
        ctx->style->colors[MU_COLOR_BUTTON] = mu_color(52, 96, 130, 255);
        ctx->style->colors[MU_COLOR_TEXT] = CLR_ACCENT;
    }
    int res = mu_button(ctx, label);
    ctx->style->colors[MU_COLOR_BUTTON] = saved_button;
    ctx->style->colors[MU_COLOR_TEXT] = saved_text;
    return (res & MU_RES_SUBMIT) != 0;
}

/** @brief Horizontal meter with a caption inside it */
static void meter(mu_Context *ctx, float fraction, mu_Color color, const char *caption) {
    mu_Rect r = mu_layout_next(ctx);
    mu_draw_rect(ctx, r, CLR_GRAPH_BG);

    int filled = (int)((float)(r.w - 2) * clampf(fraction, 0.0f, 1.0f));
    mu_draw_rect(ctx, mu_rect(r.x + 1, r.y + 1, filled, r.h - 2), color);
    mu_draw_control_text(ctx, caption, r, MU_COLOR_TEXT, MU_OPT_ALIGNCENTER);
}

/**
 * @brief Line graph with a shaded area, drawn through the custom commands
 *
 * Samples are reduced to at most one column per pixel so the command list
 * stays small no matter how long the history grows.
 */
static void line_graph(mu_Context *ctx, const char *title, const float *samples, int count,
                       mu_Color color, const char *value_caption) {
    row_full(ctx);
    mu_layout_height(ctx, (int)(GRAPH_HEIGHT * (ctx->style->size.y / 20.0f + 0.5f)));
    mu_Rect r = mu_layout_next(ctx);

    mu_draw_rect(ctx, r, CLR_GRAPH_BG);
    mu_draw_box(ctx, r, ctx->style->colors[MU_COLOR_BORDER]);

    /* Captions sit inside the frame to keep the panel compact */
    mu_draw_text(ctx, ctx->style->font, title, -1, mu_vec2(r.x + 4, r.y + 2), CLR_MUTED);
    if (value_caption) {
        mu_draw_control_text(ctx, value_caption, mu_rect(r.x, r.y + 2, r.w - 4, ctx->style->size.y),
                             MU_COLOR_TEXT, MU_OPT_ALIGNRIGHT);
    }

    if (count < 2) {
        mu_draw_control_text(ctx, "collecting data...", r, MU_COLOR_TEXT, MU_OPT_ALIGNCENTER);
        return;
    }

    float max_value = 0.0f;
    for (int i = 0; i < count; i++) {
        if (samples[i] > max_value) max_value = samples[i];
    }
    if (max_value <= 0.0f) max_value = 1.0f;

    int plot_top = r.y + ctx->style->size.y + 2;
    int plot_height = r.y + r.h - 2 - plot_top;
    if (plot_height < 8) return;
    float baseline = (float)(plot_top + plot_height);

    int columns = r.w - 8;
    if (columns > count) columns = count;
    if (columns < 2) return;

    mu_Color area = color;
    area.a = 60;

    float prev_x = 0.0f, prev_y = 0.0f;
    for (int c = 0; c < columns; c++) {
        /* Each column takes the peak of the samples it covers */
        int from = count * c / columns;
        int to = count * (c + 1) / columns;
        if (to <= from) to = from + 1;

        float peak = samples[from];
        for (int i = from + 1; i < to && i < count; i++) {
            if (samples[i] > peak) peak = samples[i];
        }

        float x = (float)(r.x + 4) + (float)c * (float)(r.w - 8) / (float)(columns - 1);
        float y = baseline - (peak / max_value) * (float)plot_height;

        ui_draw_line(ctx, x, baseline, x, y, 1.0f, area);
        if (c > 0) {
            ui_draw_line(ctx, prev_x, prev_y, x, y, 1.6f, color);
        }
        prev_x = x;
        prev_y = y;
    }
}

/* ============== Sections ============== */

static void section_simulation(App *app, mu_Context *ctx) {
    if (!mu_header_ex(ctx, "Simulation", MU_OPT_EXPANDED)) return;

    row_split(ctx, 2);
    if (mu_button(ctx, app->paused ? "Resume" : "Pause")) {
        app_do_action(app, ACTION_TOGGLE_PAUSE);
    }
    if (mu_button(ctx, "Step")) {
        app_do_action(app, ACTION_STEP_ONCE);
    }

    /* Speed presets */
    row_full(ctx);
    mu_label(ctx, "Speed");
    row_split(ctx, SPEED_LEVEL_COUNT);
    static const char *speed_labels[SPEED_LEVEL_COUNT] = {".25", ".5", "1x", "2x", "4x", "8x"};
    for (int i = 0; i < SPEED_LEVEL_COUNT; i++) {
        if (option_button(ctx, speed_labels[i], !app->max_speed && app->speed_index == i)) {
            app_set_speed_index(app, i);
        }
    }
    row_full(ctx);
    if (option_button(ctx, "Unlimited speed (for training)", app->max_speed)) {
        app_do_action(app, ACTION_SPEED_MAX);
    }

    /* Brain */
    row_full(ctx);
    mu_label(ctx, "Brain");
    row_split(ctx, 2);
    if (option_button(ctx, "Neural", app->cfg.brain_mode == BRAIN_NEURAL)) {
        app->cfg.brain_mode = BRAIN_NEURAL;
    }
    if (option_button(ctx, "Classic", app->cfg.brain_mode == BRAIN_CLASSIC)) {
        app->cfg.brain_mode = BRAIN_CLASSIC;
    }

    /* Click tool */
    row_full(ctx);
    mu_label(ctx, "Left click");
    row_split(ctx, 2);
    if (option_button(ctx, "Select ant", app->tool == TOOL_SELECT)) {
        app->tool = TOOL_SELECT;
    }
    if (option_button(ctx, "Place food", app->tool == TOOL_FOOD)) {
        app->tool = TOOL_FOOD;
    }

    /* Seed and restart */
    row_label_control(ctx, content_width(ctx) * 35 / 100);
    mu_label(ctx, "Seed");
    if (mu_textbox(ctx, app->seed_text, sizeof(app->seed_text)) & MU_RES_SUBMIT) {
        app->seed = (uint32_t)strtoul(app->seed_text, NULL, 10);
        app_reset_world(app, false);
    }

    row_split(ctx, 2);
    if (mu_button(ctx, "Restart")) {
        app->seed = (uint32_t)strtoul(app->seed_text, NULL, 10);
        app_do_action(app, ACTION_RESET);
    }
    if (mu_button(ctx, "New seed")) {
        app_do_action(app, ACTION_RESET_NEW_SEED);
    }
    row_full(ctx);
    if (mu_button(ctx, "Regenerate walls")) {
        app_do_action(app, ACTION_NEW_MAZE);
    }
}

static void section_view(App *app, mu_Context *ctx) {
    if (!mu_header(ctx, "View")) return;

    row_split(ctx, 2);
    toggle(ctx, "Food trail", &app->view.trail_food);
    toggle(ctx, "Home trail", &app->view.trail_home);
    row_split(ctx, 2);
    toggle(ctx, "Danger", &app->view.trail_danger);
    toggle(ctx, "Grid", &app->view.grid);
    row_split(ctx, 2);
    toggle(ctx, "Walls", &app->view.walls);
    toggle(ctx, "Food", &app->view.food);
    row_split(ctx, 2);
    toggle(ctx, "Deaths", &app->view.markers);
    toggle(ctx, "Headings", &app->view.ant_headings);
    row_split(ctx, 2);
    toggle(ctx, "Vision rays", &app->view.vision_rays);
    toggle(ctx, "Ants", &app->view.ants);

    slider_float(ctx, "Zoom", &app->camera.zoom, app->camera.min_zoom, app->camera.max_zoom,
                 0.0f, "%.2fx");

    row_full(ctx);
    if (mu_button(ctx, "Fit world to window")) {
        app_do_action(app, ACTION_FIT_CAMERA);
    }
}

static void section_colony(App *app, mu_Context *ctx) {
    if (!mu_header_ex(ctx, "Colony", MU_OPT_EXPANDED)) return;

    const World *w = app->world;
    const WorldStats *s = &w->stats;

    int active_food = 0;
    float food_left = 0.0f;
    for (int i = 0; i < w->food_count; i++) {
        if (w->food[i].amount > 0.0f) {
            active_food++;
            food_left += w->food[i].amount;
        }
    }

    stat_rowf(ctx, "Population", "%d", w->ant_count);
    stat_rowf(ctx, "Carrying food", "%d  (%.0f%%)", s->carrying,
              w->ant_count ? 100.0 * s->carrying / w->ant_count : 0.0);
    stat_rowf(ctx, "Food in nest", "%.0f", (double)w->nest.food_stored);
    stat_rowf(ctx, "Deliveries", "%llu", (unsigned long long)s->total_deliveries);
    stat_rowf(ctx, "Food sources", "%d  (%.0f left)", active_food, (double)food_left);
    stat_rowf(ctx, "Deaths", "%llu starved / %llu stuck",
              (unsigned long long)s->deaths_starved, (unsigned long long)s->deaths_stuck);

    row_full(ctx);
    meter(ctx, s->avg_energy / ANT_MAX_ENERGY,
          s->avg_energy > ANT_MAX_ENERGY * 0.5f ? CLR_GOOD : CLR_WARN, "average energy");

    stat_rowf(ctx, "Generation", "%d", w->evo.generation);
    stat_rowf(ctx, "Elite genomes", "%d", w->evo.elite_count);
    stat_rowf(ctx, "Best fitness", "%.1f",
              (double)(w->evo.elite_count ? w->evo.elites[0].fitness : 0.0f));

    const GenerationStats *last_gen = stats_generation(s, s->gen_count - 1);
    if (last_gen) {
        stat_rowf(ctx, "Last generation", "best %.0f / avg %.0f",
                  (double)last_gen->best_fitness, (double)last_gen->avg_fitness);
    }

    stat_rowf(ctx, "Simulated", "%.1f s (tick %llu)",
              (double)w->tick / SIM_TICK_RATE, (unsigned long long)w->tick);
    stat_rowf(ctx, "Ants on screen", "%d",
              world_renderer_last_ant_count(app->world_renderer));
    stat_rowf(ctx, "Frame rate", "%.0f fps", (double)app->fps);
    stat_rowf(ctx, "Sim rate", "%.0f ticks/s", (double)app->sim_tps);
}

static void section_graphs(App *app, mu_Context *ctx) {
    if (!mu_header_ex(ctx, "Graphs", MU_OPT_EXPANDED)) return;

    const WorldStats *s = &app->world->stats;
    static float buffer[STATS_HISTORY_LEN > GEN_HISTORY_LEN ? STATS_HISTORY_LEN
                                                            : GEN_HISTORY_LEN];
    char caption[48];

    int n = s->sample_count;
    for (int i = 0; i < n; i++) buffer[i] = stats_sample(s, i)->food_stored;
    snprintf(caption, sizeof(caption), "%.0f", (double)app->world->nest.food_stored);
    line_graph(ctx, "Food stored", buffer, n, CLR_GOOD, caption);

    /* Samples cover a fixed number of ticks, so scale to deliveries per second */
    const float per_second = (float)SIM_TICK_RATE / (float)STATS_SAMPLE_INTERVAL;
    for (int i = 0; i < n; i++) {
        buffer[i] = (float)stats_sample(s, i)->deliveries * per_second;
    }
    snprintf(caption, sizeof(caption), "%.1f /s", n ? (double)buffer[n - 1] : 0.0);
    line_graph(ctx, "Deliveries per second", buffer, n, CLR_ACCENT, caption);

    for (int i = 0; i < n; i++) buffer[i] = (float)stats_sample(s, i)->carrying;
    snprintf(caption, sizeof(caption), "%d", s->carrying);
    line_graph(ctx, "Ants carrying food", buffer, n, CLR_WARN, caption);

    int gens = s->gen_count;
    for (int i = 0; i < gens; i++) buffer[i] = stats_generation(s, i)->best_fitness;
    snprintf(caption, sizeof(caption), "%d gens", gens);
    line_graph(ctx, "Best fitness per generation", buffer, gens, CLR_POSITIVE, caption);

    for (int i = 0; i < gens; i++) buffer[i] = stats_generation(s, i)->avg_fitness;
    line_graph(ctx, "Average fitness per generation", buffer, gens, CLR_MUTED, caption);
}

static void section_parameters(App *app, mu_Context *ctx) {
    if (!mu_header(ctx, "Parameters")) return;

    SimConfig *cfg = &app->cfg;

    if (mu_begin_treenode_ex(ctx, "Ants", MU_OPT_EXPANDED)) {
        slider_int(ctx, "Population", &cfg->population, 1, MAX_POPULATION);
        slider_float(ctx, "Speed", &cfg->ant_speed, 0.2f, 6.0f, 0.1f, "%.1f px/tick");
        slider_float(ctx, "Turn rate", &cfg->turn_rate, 0.0f, 1.0f, 0.01f, "%.2f rad");
        slider_float(ctx, "Energy drain", &cfg->energy_drain, 0.0f, 0.2f, 0.001f, "%.3f /tick");
        slider_float(ctx, "Delivery energy", &cfg->delivery_energy, 0.0f, 100.0f, 1.0f, "%.0f");
        slider_float(ctx, "Trail following", &cfg->trail_sensitivity, 0.0f, 1.0f, 0.01f, "%.2f");
        row_full(ctx);
        mu_text(ctx, "Trail following only affects the classic brain.");
        mu_end_treenode(ctx);
    }

    if (mu_begin_treenode(ctx, "Pheromones")) {
        slider_float(ctx, "Deposit", &cfg->deposit_amount, 0.0f, 60.0f, 0.5f, "%.1f");
        slider_int(ctx, "Deposit every", &cfg->deposit_interval, 1, 30);
        slider_float(ctx, "Trail decay", &cfg->trail_evaporation, 0.95f, 1.0f, 0.0005f, "%.4f");
        slider_float(ctx, "Danger decay", &cfg->danger_evaporation, 0.95f, 1.0f, 0.0005f, "%.4f");
        slider_float(ctx, "Danger on death", &cfg->danger_on_death, 0.0f, 200.0f, 5.0f, "%.0f");
        row_full(ctx);
        mu_text(ctx, "Decay is the fraction kept each tick; 1.0 never fades.");
        mu_end_treenode(ctx);
    }

    if (mu_begin_treenode(ctx, "Evolution")) {
        float seconds = (float)cfg->generation_ticks / SIM_TICK_RATE;
        slider_float(ctx, "Generation", &seconds, 2.0f, 300.0f, 1.0f, "%.0f s");
        cfg->generation_ticks = (int)(seconds * SIM_TICK_RATE);

        slider_int(ctx, "Elite genomes", &cfg->elite_count, 1, ELITE_MAX);
        slider_float(ctx, "Mutation rate", &cfg->mutation_rate, 0.0f, 1.0f, 0.01f, "%.2f");
        slider_float(ctx, "Mutation size", &cfg->mutation_strength, 0.0f, 1.5f, 0.01f, "%.2f");
        slider_float(ctx, "Replaced per gen", &cfg->replace_fraction, 0.0f, 1.0f, 0.05f, "%.2f");
        row_full(ctx);
        mu_text(ctx, "At the end of each generation the weakest ants are "
                     "replaced by children of the elites.");
        mu_end_treenode(ctx);
    }

    if (mu_begin_treenode(ctx, "World (applies on restart)")) {
        slider_int(ctx, "Width", &cfg->world_width, 800, 6000);
        slider_int(ctx, "Height", &cfg->world_height, 600, 4000);
        slider_int(ctx, "Food sources", &cfg->food_source_count, 0, 80);
        slider_float(ctx, "Food min", &cfg->food_min, 10.0f, 1000.0f, 10.0f, "%.0f");
        slider_float(ctx, "Food max", &cfg->food_max, 10.0f, 1000.0f, 10.0f, "%.0f");
        mu_end_treenode(ctx);
    }

    row_full(ctx);
    if (mu_button(ctx, "Restore defaults")) {
        BrainMode mode = cfg->brain_mode;
        sim_config_defaults(cfg);
        cfg->brain_mode = mode;
    }
}

static void section_shortcuts(mu_Context *ctx) {
    if (!mu_header(ctx, "Shortcuts")) return;

    int count = 0;
    const Keybind *binds = input_keybinds(&count);
    for (int i = 0; i < count; i++) {
        int widths[2] = {content_width(ctx) * 22 / 100, -1};
        mu_layout_row(ctx, 2, widths, 0);
        mu_draw_control_text(ctx, binds[i].key_label, mu_layout_next(ctx),
                             MU_COLOR_TITLETEXT, MU_OPT_ALIGNRIGHT);
        mu_label(ctx, binds[i].description);
    }

    row_full(ctx);
    mu_text(ctx, "Drag with the right mouse button to pan, scroll to zoom, "
                 "hold Shift and click to drop food.");
}

/* ============== Neural network view ============== */

/**
 * @brief Draw the selected ant's network: inputs, hidden layer, outputs
 *
 * Connection brightness follows how much each weight contributed on the
 * last decision (weight times the activation feeding it), so what the
 * diagram shows is the signal that actually drove the ant.
 */
static void draw_network(mu_Context *ctx, const Ant *ant) {
    row_full(ctx);
    mu_layout_height(ctx, (int)(NETWORK_HEIGHT * (ctx->style->size.y / 20.0f + 0.5f)));
    mu_Rect r = mu_layout_next(ctx);

    mu_draw_rect(ctx, r, CLR_GRAPH_BG);
    mu_draw_box(ctx, r, ctx->style->colors[MU_COLOR_BORDER]);

    const NNActivations *act = &ant->nn;
    float pad = 14.0f;
    float top = (float)r.y + pad;
    float height = (float)r.h - pad * 2.0f;
    float x_in = (float)r.x + pad * 3.0f;
    float x_hid = (float)r.x + (float)r.w * 0.55f;
    float x_out = (float)r.x + (float)r.w - pad * 3.0f;

    float in_step = height / (float)(NN_INPUT_SIZE - 1);
    float hid_step = height / (float)(NN_HIDDEN_SIZE - 1);
    float out_step = height / (float)(NN_OUTPUT_SIZE + 1);
    float node_r = fmaxf(2.5f, (float)ctx->style->size.y * 0.16f);

    /* Connections, strongest last so they read on top */
    for (int i = 0; i < NN_INPUT_SIZE; i++) {
        float y_in = top + in_step * (float)i;
        float activation = act->inputs[i];
        if (fabsf(activation) < 0.02f) continue;

        for (int j = 0; j < NN_HIDDEN_SIZE; j++) {
            float signal = nn_weight_ih(&ant->genome, i, j) * activation;
            if (fabsf(signal) < 0.15f) continue;

            mu_Color c = signal > 0.0f ? CLR_POSITIVE : CLR_NEGATIVE;
            c.a = (uint8_t)clampf(fabsf(signal) * 90.0f, 20.0f, 150.0f);
            ui_draw_line(ctx, x_in, y_in, x_hid, top + hid_step * (float)j, 1.0f, c);
        }
    }
    for (int j = 0; j < NN_HIDDEN_SIZE; j++) {
        float y_hid = top + hid_step * (float)j;
        for (int k = 0; k < NN_OUTPUT_SIZE; k++) {
            float signal = nn_weight_ho(&ant->genome, j, k) * act->hidden[j];
            if (fabsf(signal) < 0.15f) continue;

            mu_Color c = signal > 0.0f ? CLR_POSITIVE : CLR_NEGATIVE;
            c.a = (uint8_t)clampf(fabsf(signal) * 90.0f, 20.0f, 170.0f);
            ui_draw_line(ctx, x_hid, y_hid, x_out, top + out_step * (float)(k + 1), 1.2f, c);
        }
    }

    /* Nodes, shaded by activation */
    for (int i = 0; i < NN_INPUT_SIZE; i++) {
        float v = clampf(act->inputs[i], -1.0f, 1.0f);
        mu_Color c = v >= 0.0f ? CLR_GOOD : CLR_NEGATIVE;
        c.a = (uint8_t)(60.0f + 195.0f * fabsf(v));
        ui_draw_disc(ctx, x_in, top + in_step * (float)i, node_r, c);
    }
    for (int j = 0; j < NN_HIDDEN_SIZE; j++) {
        float v = clampf(act->hidden[j], -1.0f, 1.0f);
        mu_Color c = v >= 0.0f ? CLR_ACCENT : CLR_NEGATIVE;
        c.a = (uint8_t)(60.0f + 195.0f * fabsf(v));
        ui_draw_disc(ctx, x_hid, top + hid_step * (float)j, node_r * 1.2f, c);
    }
    for (int k = 0; k < NN_OUTPUT_SIZE; k++) {
        float v = clampf(act->outputs[k], -1.0f, 1.0f);
        mu_Color c = v >= 0.0f ? CLR_GOOD : CLR_NEGATIVE;
        c.a = (uint8_t)(80.0f + 175.0f * fabsf(v));
        float y = top + out_step * (float)(k + 1);
        ui_draw_disc(ctx, x_out, y, node_r * 1.6f, c);
        mu_draw_text(ctx, ctx->style->font, nn_output_label(k), -1,
                     mu_vec2((int)(x_out - 60.0f), (int)(y - (float)ctx->style->size.y * 0.5f)),
                     CLR_MUTED);
    }

    /* Group captions for the three blocks of vision rays */
    static const char *groups[3] = {"wall", "ant", "food"};
    for (int g = 0; g < 3; g++) {
        float y = top + in_step * (float)(g * NN_NUM_VISION_RAYS + NN_NUM_VISION_RAYS / 2);
        mu_draw_text(ctx, ctx->style->font, groups[g], -1,
                     mu_vec2(r.x + 3, (int)(y - (float)ctx->style->size.y * 0.5f)), CLR_MUTED);
    }
    float y_state = top + in_step * (float)(NN_VISION_INPUTS + NN_STATE_INPUTS / 2);
    mu_draw_text(ctx, ctx->style->font, "state", -1,
                 mu_vec2(r.x + 3, (int)(y_state - (float)ctx->style->size.y * 0.5f)), CLR_MUTED);
}

/** @brief Pull a panel back into view after the window shrank */
static void keep_on_screen(mu_Context *ctx, const char *name, const App *app) {
    mu_Container *cnt = mu_get_container(ctx, name);
    if (!cnt || cnt->rect.w == 0) return;

    int max_x = app->window_width - cnt->rect.w;
    int max_y = app->window_height - ctx->style->title_height;
    if (cnt->rect.x > max_x) cnt->rect.x = max_x > 0 ? max_x : 0;
    if (cnt->rect.y > max_y) cnt->rect.y = max_y > 0 ? max_y : 0;
    if (cnt->rect.h > app->window_height) cnt->rect.h = app->window_height;
}

static void panel_inspector(App *app, mu_Context *ctx) {
    int index = app_selected_index(app);
    keep_on_screen(ctx, "Ant inspector", app);

    float scale = app->ui_scale;
    int width = (int)(RIGHT_PANEL_WIDTH * scale);
    mu_Rect rect = mu_rect(app->window_width - width - (int)(PANEL_MARGIN * scale),
                           (int)(PANEL_MARGIN * scale), width,
                           (int)(app->window_height * 0.75f));

    /* microui keeps a container's rect once it exists, so the height has to
       be set explicitly when the panel switches between hint and inspector. */
    int hint_height = (int)(90 * scale);
    static bool showed_ant = false;
    if (showed_ant != (index >= 0)) {
        mu_Container *cnt = mu_get_container(ctx, "Ant inspector");
        if (cnt->rect.w != 0) {
            cnt->rect.h = (index >= 0) ? rect.h : hint_height;
        }
        showed_ant = (index >= 0);
    }

    if (index < 0) {
        /* Nothing selected: keep the hint compact */
        if (mu_begin_window_ex(ctx, "Ant inspector",
                               mu_rect(rect.x, rect.y, width, hint_height),
                               MU_OPT_NOCLOSE | MU_OPT_NORESIZE)) {
            row_full(ctx);
            mu_text(ctx, "Click an ant to inspect its state and brain.");
            mu_end_window(ctx);
        }
        return;
    }

    const Ant *ant = &app->world->ants[index];
    if (!mu_begin_window_ex(ctx, "Ant inspector", rect, MU_OPT_NOCLOSE)) return;

    stat_rowf(ctx, "Ant", "#%u", ant->id);
    stat_row(ctx, "State", ant->state == ANT_STATE_RETURNING ? "returning with food"
                                                             : "foraging");
    stat_rowf(ctx, "Position", "%.0f, %.0f", (double)ant->pos.x, (double)ant->pos.y);
    stat_rowf(ctx, "Heading", "%.0f deg", (double)(ant->heading * 180.0f / PI_F));
    stat_rowf(ctx, "Speed", "%.2f px/tick", (double)ant->speed);
    stat_rowf(ctx, "Age", "%.1f s", (double)ant->age / SIM_TICK_RATE);
    stat_rowf(ctx, "Deliveries", "%d lifetime", ant->deliveries);
    stat_rowf(ctx, "This generation", "%d", ant->gen_deliveries);
    stat_rowf(ctx, "Fitness", "%.1f", (double)ant_fitness(ant));

    row_full(ctx);
    char energy_caption[32];
    snprintf(energy_caption, sizeof(energy_caption), "energy %.0f%%",
             (double)(ant->energy / ANT_MAX_ENERGY * 100.0f));
    meter(ctx, ant->energy / ANT_MAX_ENERGY,
          ant->energy > ANT_MAX_ENERGY * 0.3f ? CLR_GOOD : CLR_BAD, energy_caption);

    row_split(ctx, 2);
    if (option_button(ctx, "Follow", app->follow_selected)) {
        app_do_action(app, ACTION_FOLLOW_SELECTED);
    }
    if (mu_button(ctx, "Deselect")) {
        app->selected_id = 0;
        app->follow_selected = false;
    }

    if (mu_header_ex(ctx, "Brain", MU_OPT_EXPANDED)) {
        if (app->cfg.brain_mode == BRAIN_NEURAL) {
            draw_network(ctx, ant);

            for (int k = 0; k < NN_OUTPUT_SIZE; k++) {
                row_label_control(ctx, content_width(ctx) * 40 / 100);
                mu_label(ctx, nn_output_label(k));

                /* Outputs run -1..1, so the meter shows the shifted value */
                char caption[24];
                snprintf(caption, sizeof(caption), "%+.2f", (double)ant->nn.outputs[k]);
                meter(ctx, (ant->nn.outputs[k] + 1.0f) * 0.5f, CLR_ACCENT, caption);
            }
            row_full(ctx);
            mu_text(ctx, "Blue lines push a neuron up, orange pushes it down; "
                         "brightness is how much that link mattered last tick.");
        } else {
            row_full(ctx);
            mu_text(ctx, "The classic brain follows trails directly and has no "
                         "network. Switch the brain to Neural to see one.");
        }
    }

    mu_end_window(ctx);
}

/* ============== Entry point ============== */

static void panel_controls(App *app, mu_Context *ctx) {
    float scale = app->ui_scale;
    int margin = (int)(PANEL_MARGIN * scale);
    mu_Rect rect = mu_rect(margin, margin, (int)(LEFT_PANEL_WIDTH * scale),
                           app->window_height - margin * 2);

    keep_on_screen(ctx, "Ant Colony", app);
    if (!mu_begin_window_ex(ctx, "Ant Colony", rect, MU_OPT_NOCLOSE)) return;

    /* Status line: what the simulation is doing right now */
    char status[96];
    const char *state = app->paused ? "PAUSED" : (app->max_speed ? "UNLIMITED" : "RUNNING");
    if (app->paused) {
        snprintf(status, sizeof(status), "%s  -  tick %llu  -  generation %d",
                 state, (unsigned long long)app->world->tick, app->world->evo.generation);
    } else {
        snprintf(status, sizeof(status), "%s %.2gx  -  tick %llu  -  generation %d",
                 state, (double)app_sim_speed(app),
                 (unsigned long long)app->world->tick, app->world->evo.generation);
    }
    row_full(ctx);
    mu_Rect status_rect = mu_layout_next(ctx);
    mu_draw_rect(ctx, status_rect, CLR_GRAPH_BG);
    mu_draw_control_text(ctx, status, status_rect,
                         app->paused ? MU_COLOR_TEXT : MU_COLOR_TITLETEXT, MU_OPT_ALIGNCENTER);

    section_simulation(app, ctx);
    section_colony(app, ctx);
    section_graphs(app, ctx);
    section_view(app, ctx);
    section_parameters(app, ctx);
    section_shortcuts(ctx);

    mu_end_window(ctx);
}

void ui_panels_build(App *app) {
    mu_Context *ctx = ui_context();
    panel_controls(app, ctx);
    panel_inspector(app, ctx);
}
