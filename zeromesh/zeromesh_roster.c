#include "zeromesh_roster.h"
#include "zeromesh_gui.h"
#include <gui/elements.h>
#include <stdio.h>
#include <string.h>

void roster_add_node(ZeroMeshApp* app, uint32_t node_id, int8_t snr, int16_t rssi) {
    if(!app || node_id == 0 || node_id == 0xFFFFFFFF) return;
    
    furi_mutex_acquire(app->lock, FuriWaitForever);
    
    uint8_t target_idx = 255;
    uint32_t oldest_time = 0xFFFFFFFF;
    uint8_t oldest_idx = 0;
    
    for(uint8_t i = 0; i < app->roster.count; i++) {
        if(app->roster.nodes[i].node_id == node_id) {
            target_idx = i;
            break;
        }
        if(app->roster.nodes[i].last_seen < oldest_time) {
            oldest_time = app->roster.nodes[i].last_seen;
            oldest_idx = i;
        }
    }
    
    if(target_idx == 255) {
        if(app->roster.count < ROSTER_MAX_NODES) {
            target_idx = app->roster.count;
            app->roster.count++;
        } else {
            target_idx = oldest_idx;
        }
        app->roster.nodes[target_idx].node_id = node_id;
        app->roster.nodes[target_idx].has_telemetry = false;
        app->roster.nodes[target_idx].has_new_dm = false;
    }
    
    app->roster.nodes[target_idx].last_seen = furi_get_tick() / 1000;
    app->roster.nodes[target_idx].last_snr = snr;
    app->roster.nodes[target_idx].last_rssi = rssi;
    
    furi_mutex_release(app->lock);
}

void roster_update_telemetry(ZeroMeshApp* app, uint32_t node_id, uint8_t battery_level, float voltage) {
    if(!app || node_id == 0) return;
    
    furi_mutex_acquire(app->lock, FuriWaitForever);
    for(uint8_t i = 0; i < app->roster.count; i++) {
        if(app->roster.nodes[i].node_id == node_id) {
            app->roster.nodes[i].battery_level = battery_level;
            app->roster.nodes[i].voltage = voltage;
            app->roster.nodes[i].has_telemetry = true;
            break;
        }
    }
    furi_mutex_release(app->lock);
}

static void draw_roster_bubble(Canvas* canvas, int x, int y, int max_w, const char* text, bool is_tx) {
    canvas_set_font(canvas, FontSecondary);
    int text_w = canvas_string_width(canvas, text);
    if(text_w > max_w - 8) text_w = max_w - 8;
    int bubble_w = text_w + 8;
    int bubble_h = 12;

    if(is_tx) {
        int bx = x + max_w - bubble_w;
        canvas_set_color(canvas, ColorBlack);
        canvas_draw_rbox(canvas, bx, y, bubble_w, bubble_h, 3);
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_str(canvas, bx + 4, y + 9, text);
        canvas_set_color(canvas, ColorBlack);
    } else {
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_rbox(canvas, x, y, bubble_w, bubble_h, 3);
        canvas_set_color(canvas, ColorBlack);
        canvas_draw_rframe(canvas, x, y, bubble_w, bubble_h, 3);
        canvas_draw_str(canvas, x + 4, y + 9, text);
    }
}

void render_roster(Canvas* canvas, ZeroMeshApp* app) {
    if(app->roster.count == 0) {
        draw_header(canvas, app, "Node Roster");
        canvas_set_color(canvas, ColorBlack);
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 16, 34, "No nodes discovered");
        return;
    }
    
    NodeEntry* selected = &app->roster.nodes[app->roster.selected_idx];
    char title_buf[32];
    
    if(app->roster.state == RosterStateList) {
        draw_header(canvas, app, "Node Roster");
        canvas_set_color(canvas, ColorBlack);
        canvas_set_font(canvas, FontSecondary);
        
        int y = 24;
        for(uint8_t i = 0; i < 4 && i < app->roster.count; i++) {
            uint8_t idx = (app->roster.selected_idx / 4) * 4 + i;
            if(idx >= app->roster.count) break;
            
            if(idx == app->roster.selected_idx) {
                canvas_set_color(canvas, ColorBlack);
                canvas_draw_box(canvas, 0, y - 8, 128, 11);
                canvas_set_color(canvas, ColorWhite);
            } else {
                canvas_set_color(canvas, ColorBlack);
            }
            
            char line_buf[64];
            uint32_t now = furi_get_tick() / 1000;
            uint32_t diff = now - app->roster.nodes[idx].last_seen;
            const char* alert = app->roster.nodes[idx].has_new_dm ? "(!)" : "   ";
            snprintf(line_buf, sizeof(line_buf), "%s %08lX  %lus ago", alert, (unsigned long)app->roster.nodes[idx].node_id, (unsigned long)diff);
            canvas_draw_str(canvas, 4, y, line_buf);
            y += 12;
        }
        canvas_set_color(canvas, ColorBlack);
        canvas_draw_str(canvas, 2, 64, "Hold OK: Info");
        canvas_draw_str(canvas, 92, 64, "OK: Chat");
    } 
    else if(app->roster.state == RosterStateChat) {
        snprintf(title_buf, sizeof(title_buf), "Chat: %08lX", (unsigned long)selected->node_id);
        draw_header(canvas, app, title_buf);
        canvas_set_color(canvas, ColorBlack);
        
        uint8_t chat_msgs[MSG_HISTORY];
        uint8_t chat_count = 0;
        
        for(uint8_t i = 0; i < app->history.count; i++) {
            uint8_t idx = (app->history.head + MSG_HISTORY - app->history.count + i) % MSG_HISTORY;
            Message* m = &app->history.msgs[idx];
            
            bool is_dm_from_them = (m->from == selected->node_id && m->to == app->my_node_num);
            bool is_dm_from_us = (m->from == app->my_node_num && m->to == selected->node_id);

            if(is_dm_from_them || is_dm_from_us) {
                chat_msgs[chat_count++] = idx;
            }
        }
        
        if(chat_count == 0) {
            canvas_set_font(canvas, FontSecondary);
            canvas_draw_str(canvas, 20, 34, "No direct messages");
        } else {
            int max_visible = 3;
            if(app->roster.chat_scroll > chat_count - max_visible && chat_count > max_visible) {
                app->roster.chat_scroll = chat_count - max_visible;
            } else if (chat_count <= max_visible) {
                app->roster.chat_scroll = 0;
            }
            
            uint8_t start_offset = 0;
            if(chat_count > max_visible) {
                start_offset = chat_count - max_visible - app->roster.chat_scroll;
            }
            
            int y = 18;
            for(int i = 0; i < max_visible && (start_offset + i) < chat_count; i++) {
                uint8_t idx = chat_msgs[start_offset + i];
                Message* msg = &app->history.msgs[idx];
                draw_roster_bubble(canvas, 2, y, 124, msg->text, msg->is_tx);
                y += 14;
            }
        }
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 2, 64, "< Back");
        canvas_draw_str(canvas, 98, 64, "OK: TX");
    }
    else if(app->roster.state == RosterStateDetails) {
        snprintf(title_buf, sizeof(title_buf), "Node: %08lX", (unsigned long)selected->node_id);
        draw_header(canvas, app, title_buf);
        canvas_set_color(canvas, ColorBlack);
        canvas_set_font(canvas, FontSecondary);
        
        char buf[64];
        uint32_t now = furi_get_tick() / 1000;
        uint32_t diff = now - selected->last_seen;
        
        snprintf(buf, sizeof(buf), "Last Seen: %lus ago", (unsigned long)diff);
        canvas_draw_str(canvas, 4, 24, buf);
        
        snprintf(buf, sizeof(buf), "Signal: SNR %d / RSSI %d", selected->last_snr, selected->last_rssi);
        canvas_draw_str(canvas, 4, 36, buf);
        
        if(selected->has_telemetry) {
            snprintf(buf, sizeof(buf), "Battery: %u%%", selected->battery_level);
            canvas_draw_str(canvas, 4, 48, buf);
            
            int v_int = (int)selected->voltage;
            int v_dec = (int)(selected->voltage * 100) % 100;
            snprintf(buf, sizeof(buf), "Voltage: %d.%02dV", v_int, v_dec);
            canvas_draw_str(canvas, 4, 60, buf);
        } else {
            canvas_draw_str(canvas, 4, 48, "No telemetry data yet");
        }
    }
}

void input_roster(InputEvent* e, ZeroMeshApp* app) {
    if(!app || app->roster.count == 0) return;
    
    if(app->roster.state == RosterStateList) {
        if(e->key == InputKeyUp && (e->type == InputTypeShort || e->type == InputTypeRepeat)) {
            if(app->roster.selected_idx > 0) app->roster.selected_idx--;
            else app->roster.selected_idx = app->roster.count - 1;
            view_port_update(app->vp);
        }
        else if(e->key == InputKeyDown && (e->type == InputTypeShort || e->type == InputTypeRepeat)) {
            if(app->roster.selected_idx < app->roster.count - 1) app->roster.selected_idx++;
            else app->roster.selected_idx = 0;
            view_port_update(app->vp);
        }
        else if(e->key == InputKeyOk) {
            if(e->type == InputTypeShort) {
                app->roster.nodes[app->roster.selected_idx].has_new_dm = false;
                app->roster.state = RosterStateChat;
                app->roster.chat_scroll = 0;
                view_port_update(app->vp);
            } else if(e->type == InputTypeLong) {
                app->roster.state = RosterStateDetails;
                view_port_update(app->vp);
            }
        }
    }
    else if(app->roster.state == RosterStateChat) {
        if(e->key == InputKeyUp && (e->type == InputTypeShort || e->type == InputTypeRepeat)) {
            app->roster.chat_scroll++;
            view_port_update(app->vp);
        }
        else if(e->key == InputKeyDown && (e->type == InputTypeShort || e->type == InputTypeRepeat)) {
            if(app->roster.chat_scroll > 0) app->roster.chat_scroll--;
            view_port_update(app->vp);
        }
        else if(e->key == InputKeyOk && e->type == InputTypeShort) {
            app->show_keyboard = true;
        }
        else if(e->key == InputKeyBack && e->type == InputTypeShort) {
            app->roster.state = RosterStateList;
            view_port_update(app->vp);
        }
    }
    else if(app->roster.state == RosterStateDetails) {
        if(e->key == InputKeyBack && e->type == InputTypeShort) {
            app->roster.state = RosterStateList;
            view_port_update(app->vp);
        }
    }
}
