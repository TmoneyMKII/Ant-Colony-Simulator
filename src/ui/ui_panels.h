/**
 * @file ui_panels.h
 * @brief The panel layout: controls, stats, graphs, parameters, inspector
 */

#ifndef UI_PANELS_H
#define UI_PANELS_H

struct App;

/** @brief Build every panel for this frame (call between ui_begin/ui_end) */
void ui_panels_build(struct App *app);

#endif /* UI_PANELS_H */
