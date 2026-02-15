#pragma once

#include "zeromesh_serial.h"

void draw_header(Canvas* canvas, ZeroMeshApp* app, const char* title);
void render_cb(Canvas* canvas, void* ctx);
void input_cb(InputEvent* e, void* ctx);
void text_input_callback(void* ctx);
uint32_t kb_back_callback(void* ctx);