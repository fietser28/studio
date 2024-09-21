#include <stdio.h>
#include <vector>
#include <map>
#include <emscripten.h>

#include <eez/core/os.h>
#include <eez/core/assets.h>
#include <eez/core/action.h>
#include <eez/core/vars.h>
#include <eez/core/util.h>

#include <eez/flow/flow.h>
#include <eez/flow/expression.h>
#include <eez/flow/hooks.h>
#include <eez/flow/debugger.h>
#include <eez/flow/components.h>
#include <eez/flow/flow_defs_v3.h>
#include <eez/flow/operations.h>
#include <eez/flow/lvgl_api.h>
#include <eez/flow/date.h>

#include "flow.h"

////////////////////////////////////////////////////////////////////////////////

bool is_editor = false;

uint32_t screenLoad_animType = 0;
uint32_t screenLoad_speed = 0;
uint32_t screenLoad_delay = 0;

////////////////////////////////////////////////////////////////////////////////

#define WIDGET_TIMELINE_PROPERTY_X (1 << 0)
#define WIDGET_TIMELINE_PROPERTY_Y (1 << 1)
#define WIDGET_TIMELINE_PROPERTY_WIDTH (1 << 2)
#define WIDGET_TIMELINE_PROPERTY_HEIGHT (1 << 3)
#define WIDGET_TIMELINE_PROPERTY_OPACITY (1 << 4)
#define WIDGET_TIMELINE_PROPERTY_SCALE (1 << 5)
#define WIDGET_TIMELINE_PROPERTY_ROTATE (1 << 6)
#define WIDGET_TIMELINE_PROPERTY_CP1 (1 << 7)
#define WIDGET_TIMELINE_PROPERTY_CP2 (1 << 8)

#define EASING_FUNC_LINEAR 0
#define EASING_FUNC_IN_QUAD 1
#define EASING_FUNC_OUT_QUAD 2
#define EASING_FUNC_IN_OUT_QUAD 3
#define EASING_FUNC_IN_CUBIC 4
#define EASING_FUNC_OUT_CUBIC 5
#define EASING_FUNC_IN_OUT_CUBIC 6
#define EASING_FUNC_IN__QUART 7
#define EASING_FUNC_OUT_QUART 8
#define EASING_FUNC_IN_OUT_QUART 9
#define EASING_FUNC_IN_QUINT 10
#define EASING_FUNC_OUT_QUINT 11
#define EASING_FUNC_IN_OUT_QUINT 12
#define EASING_FUNC_IN_SINE 13
#define EASING_FUNC_OUT_SINE 14
#define EASING_FUNC_IN_OUT_SINE 15
#define EASING_FUNC_IN_EXPO 16
#define EASING_FUNC_OUT_EXPO 17
#define EASING_FUNC_IN_OUT_EXPO 18
#define EASING_FUNC_IN_CIRC 19
#define EASING_FUNC_OUT_CIRC 20
#define EASING_FUNC_IN_OUT_CIRC 21
#define EASING_FUNC_IN_BACK 22
#define EASING_FUNC_OUT_BACK 23
#define EASING_FUNC_IN_OUT_BACK 24
#define EASING_FUNC_IN_ELASTIC 25
#define EASING_FUNC_OUT_ELASTIC 26
#define EASING_FUNC_IN_OUT_ELASTIC 27
#define EASING_FUNC_IN_BOUNCE 28
#define EASING_FUNC_OUT_BOUNCE 29
#define EASING_FUNC_IN_OUT_BOUNCE 30

struct TimelineKeyframe {
    float start;
    float end;

    uint32_t enabledProperties;

	int16_t x;
    uint8_t xEasingFunc;

	int16_t y;
    uint8_t yEasingFunc;

	int16_t width;
    uint8_t widthEasingFunc;

	int16_t height;
    uint8_t heightEasingFunc;

    float opacity;
    uint8_t opacityEasingFunc;

    int16_t scale;
    uint8_t scaleEasingFunc;

    int16_t rotate;
    uint8_t rotateEasingFunc;

    int32_t cp1x;
    int32_t cp1y;
    int32_t cp2x;
    int32_t cp2y;
};

struct WidgetTimeline {
    lv_obj_t *obj;
    void *flowState;

    float lastTimelinePosition;

	int16_t x;
	int16_t y;
	int16_t width;
	int16_t height;
    int16_t opacity;
    int16_t scale;
    int16_t rotate;

    std::vector<TimelineKeyframe> timeline;
};

std::vector<WidgetTimeline> widgetTimelines;

void addTimelineKeyframe(
    lv_obj_t *obj,
    void *flowState,
    float start, float end,
    uint32_t enabledProperties,
    int16_t x, uint8_t xEasingFunc,
    int16_t y, uint8_t yEasingFunc,
    int16_t width, uint8_t widthEasingFunc,
    int16_t height, uint8_t heightEasingFunc,
    int16_t opacity, uint8_t opacityEasingFunc,
    int16_t scale, uint8_t scaleEasingFunc,
    int16_t rotate, uint8_t rotateEasingFunc,
    int32_t cp1x, int32_t cp1y, int32_t cp2x, int32_t cp2y
) {
    TimelineKeyframe timelineKeyframe;

    timelineKeyframe.start = start;
    timelineKeyframe.end = end;

    timelineKeyframe.enabledProperties = enabledProperties;

	timelineKeyframe.x = x;
    timelineKeyframe.xEasingFunc = xEasingFunc;

	timelineKeyframe.y = y;
    timelineKeyframe.yEasingFunc = yEasingFunc;

	timelineKeyframe.width = width;
    timelineKeyframe.widthEasingFunc = widthEasingFunc;

	timelineKeyframe.height = height;
    timelineKeyframe.heightEasingFunc = heightEasingFunc;

    timelineKeyframe.opacity = opacity;
    timelineKeyframe.opacityEasingFunc = opacityEasingFunc;

    timelineKeyframe.scale = scale;
    timelineKeyframe.scaleEasingFunc = scaleEasingFunc;

    timelineKeyframe.rotate = rotate;
    timelineKeyframe.rotateEasingFunc = rotateEasingFunc;

    timelineKeyframe.cp1x = cp1x;
    timelineKeyframe.cp1y = cp1y;
    timelineKeyframe.cp2x = cp2x;
    timelineKeyframe.cp2y = cp2y;

    for (auto it = widgetTimelines.begin(); it != widgetTimelines.end(); it++) {
        WidgetTimeline &widgetTimeline = *it;
        if (widgetTimeline.obj == obj) {
            widgetTimeline.timeline.push_back(timelineKeyframe);
            return;
        }
    }

    WidgetTimeline widgetTimeline;
    widgetTimeline.obj = obj;
    widgetTimeline.lastTimelinePosition = -1;
    widgetTimeline.flowState = flowState;

    widgetTimeline.timeline.push_back(timelineKeyframe);

    widgetTimelines.push_back(widgetTimeline);
}

void updateTimelineProperties(WidgetTimeline &widgetTimeline, float timelinePosition) {
    if (widgetTimeline.lastTimelinePosition == -1) {
        widgetTimeline.x = lv_obj_get_style_prop(widgetTimeline.obj, LV_PART_MAIN, LV_STYLE_X).num;
        widgetTimeline.y = lv_obj_get_style_prop(widgetTimeline.obj, LV_PART_MAIN, LV_STYLE_Y).num;
        widgetTimeline.width = lv_obj_get_style_prop(widgetTimeline.obj, LV_PART_MAIN, LV_STYLE_WIDTH).num;
        widgetTimeline.height = lv_obj_get_style_prop(widgetTimeline.obj, LV_PART_MAIN, LV_STYLE_HEIGHT).num;
        widgetTimeline.opacity = lv_obj_get_style_prop(widgetTimeline.obj, LV_PART_MAIN, LV_STYLE_OPA).num / 255.0f;

#if LVGL_VERSION_MAJOR >= 9
        // TODO LVGL 9.0
        widgetTimeline.scale = lv_obj_get_style_prop(widgetTimeline.obj, LV_PART_MAIN, LV_STYLE_TRANSFORM_SCALE_X).num;
        widgetTimeline.rotate = lv_obj_get_style_prop(widgetTimeline.obj, LV_PART_MAIN, LV_STYLE_TRANSFORM_ROTATION).num;
#else
        widgetTimeline.scale = lv_obj_get_style_prop(widgetTimeline.obj, LV_PART_MAIN, LV_STYLE_TRANSFORM_ZOOM).num;
        widgetTimeline.rotate = lv_obj_get_style_prop(widgetTimeline.obj, LV_PART_MAIN, LV_STYLE_TRANSFORM_ANGLE).num;
#endif

        widgetTimeline.lastTimelinePosition = 0;
    }

    if (timelinePosition == widgetTimeline.lastTimelinePosition) {
        return;
    }

    widgetTimeline.lastTimelinePosition = timelinePosition;

    float x = widgetTimeline.x;
    float y = widgetTimeline.y;
    float w = widgetTimeline.width;
    float h = widgetTimeline.height;
    float opacity = widgetTimeline.opacity;
    float scale = widgetTimeline.scale;
    float rotate = widgetTimeline.rotate;

    bool setX = false;
    bool setY = false;
    bool setWidth = false;
    bool setHeight = false;
    bool setOPA = false;
    bool setScale = false;
    bool setRotate = false;

    for (auto itKeyframe = widgetTimeline.timeline.begin(); itKeyframe != widgetTimeline.timeline.end(); itKeyframe++) {
        TimelineKeyframe &keyframe = *itKeyframe;

        if (timelinePosition < keyframe.start) {
            continue;
        }

        if (timelinePosition >= keyframe.start && timelinePosition <= keyframe.end) {
            auto t =
                keyframe.start == keyframe.end
                    ? 1
                    : (timelinePosition - keyframe.start) /
                    (keyframe.end - keyframe.start);

            if (keyframe.enabledProperties & WIDGET_TIMELINE_PROPERTY_X) {
                auto t2 = eez::g_easingFuncs[keyframe.xEasingFunc](t);

                if (keyframe.enabledProperties & WIDGET_TIMELINE_PROPERTY_CP2) {
                    auto p1 = x;
                    auto p2 = keyframe.cp1x;
                    auto p3 = keyframe.cp2x;
                    auto p4 = keyframe.x;
                    x =
                        (1 - t2) * (1 - t2) * (1 - t2) * p1 +
                        3 * (1 - t2) * (1 - t2) * t2 * p2 +
                        3 * (1 - t2) * t2 * t2 * p3 +
                        t2 * t2 * t2 * p4;
                } else if (keyframe.enabledProperties & WIDGET_TIMELINE_PROPERTY_CP1) {
                    auto p1 = x;
                    auto p2 = keyframe.cp1x;
                    auto p3 = keyframe.x;
                    x =
                        (1 - t2) * (1 - t2) * p1 +
                        2 * (1 - t2) * t2 * p2 +
                        t2 * t2 * p3;
                } else {
                    auto p1 = x;
                    auto p2 = keyframe.x;
                    x = (1 - t2) * p1 + t2 * p2;
                }

                setX = true;
            }

            if (keyframe.enabledProperties & WIDGET_TIMELINE_PROPERTY_WIDTH) {
                w += eez::g_easingFuncs[keyframe.widthEasingFunc](t) * (keyframe.width - w);

                setWidth = true;
            }

            if (keyframe.enabledProperties & WIDGET_TIMELINE_PROPERTY_Y) {
                auto t2 = eez::g_easingFuncs[keyframe.yEasingFunc](t);

                if (keyframe.enabledProperties & WIDGET_TIMELINE_PROPERTY_CP2) {
                    auto p1 = y;
                    auto p2 = keyframe.cp1y;
                    auto p3 = keyframe.cp2y;
                    auto p4 = keyframe.y;
                    y =
                        (1 - t2) * (1 - t2) * (1 - t2) * p1 +
                        3 * (1 - t2) * (1 - t2) * t2 * p2 +
                        3 * (1 - t2) * t2 * t2 * p3 +
                        t2 * t2 * t2 * p4;
                } else if (keyframe.enabledProperties & WIDGET_TIMELINE_PROPERTY_CP1) {
                    auto p1 = y;
                    auto p2 = keyframe.cp1y;
                    auto p3 = keyframe.y;
                    y =
                        (1 - t2) * (1 - t2) * p1 +
                        2 * (1 - t2) * t2 * p2 +
                        t2 * t2 * p3;
                } else {
                    auto p1 = y;
                    auto p2 = keyframe.y;
                    y = (1 - t2) * p1 + t2 * p2;
                }

                setY = true;
            }

            if (keyframe.enabledProperties & WIDGET_TIMELINE_PROPERTY_HEIGHT) {
                h += eez::g_easingFuncs[keyframe.heightEasingFunc](t) * (keyframe.height - h);

                setHeight = true;
            }

            if (keyframe.enabledProperties & WIDGET_TIMELINE_PROPERTY_OPACITY) {
                opacity += eez::g_easingFuncs[keyframe.opacityEasingFunc](t) * (keyframe.opacity - opacity);

                setOPA = true;
            }

            if (keyframe.enabledProperties & WIDGET_TIMELINE_PROPERTY_SCALE) {
                scale += eez::g_easingFuncs[keyframe.scaleEasingFunc](t) * (keyframe.scale - scale);

                setScale = true;
            }

            if (keyframe.enabledProperties & WIDGET_TIMELINE_PROPERTY_ROTATE) {
                rotate += eez::g_easingFuncs[keyframe.rotateEasingFunc](t) * (keyframe.rotate - rotate);

                setRotate = true;
            }

            break;
        }

        if (keyframe.enabledProperties & WIDGET_TIMELINE_PROPERTY_X) {
            x = keyframe.x;
        }
        if (keyframe.enabledProperties & WIDGET_TIMELINE_PROPERTY_Y) {
            y = keyframe.y;
        }
        if (keyframe.enabledProperties & WIDGET_TIMELINE_PROPERTY_WIDTH) {
            w = keyframe.width;
        }
        if (keyframe.enabledProperties & WIDGET_TIMELINE_PROPERTY_HEIGHT) {
            h = keyframe.height;
        }

        if (keyframe.enabledProperties & WIDGET_TIMELINE_PROPERTY_OPACITY) {
            opacity = keyframe.opacity;
        }

        if (keyframe.enabledProperties & WIDGET_TIMELINE_PROPERTY_SCALE) {
            scale = keyframe.scale;
        }

        if (keyframe.enabledProperties & WIDGET_TIMELINE_PROPERTY_ROTATE) {
            rotate = keyframe.rotate;
        }
    }

    lv_style_value_t value;

    if (setX) {
        value.num = (int16_t)roundf(x);
        lv_obj_set_local_style_prop(widgetTimeline.obj, LV_STYLE_X, value, LV_PART_MAIN);
    }

    if (setY) {
        value.num = (int16_t)roundf(y);
        lv_obj_set_local_style_prop(widgetTimeline.obj, LV_STYLE_Y, value, LV_PART_MAIN);
    }

    if (setWidth) {
        value.num = (int16_t)roundf(w);
        lv_obj_set_local_style_prop(widgetTimeline.obj, LV_STYLE_WIDTH, value, LV_PART_MAIN);
    }

    if (setHeight) {
        value.num = (int16_t)roundf(h);
        lv_obj_set_local_style_prop(widgetTimeline.obj, LV_STYLE_HEIGHT, value, LV_PART_MAIN);
    }

    if (setOPA) {
        value.num = (int32_t)roundf(opacity * 255.0f);
        lv_obj_set_local_style_prop(widgetTimeline.obj, LV_STYLE_OPA, value, LV_PART_MAIN);
    }

    if (setScale) {
        value.num = (int32_t)roundf(scale);
#if LVGL_VERSION_MAJOR >= 9
        lv_obj_set_local_style_prop(widgetTimeline.obj, LV_STYLE_TRANSFORM_SCALE_X, value, LV_PART_MAIN);
        lv_obj_set_local_style_prop(widgetTimeline.obj, LV_STYLE_TRANSFORM_SCALE_Y, value, LV_PART_MAIN);
#else
        lv_obj_set_local_style_prop(widgetTimeline.obj, LV_STYLE_TRANSFORM_ZOOM, value, LV_PART_MAIN);
#endif
    }

    if (setRotate) {
        value.num = (int32_t)roundf(rotate);
#if LVGL_VERSION_MAJOR >= 9
        lv_obj_set_local_style_prop(widgetTimeline.obj, LV_STYLE_TRANSFORM_ROTATION, value, LV_PART_MAIN);
#else
        lv_obj_set_local_style_prop(widgetTimeline.obj, LV_STYLE_TRANSFORM_ANGLE, value, LV_PART_MAIN);
#endif
    }
}

void doAnimateFlowState(eez::flow::FlowState *flowState) {
    for (auto it = widgetTimelines.begin(); it != widgetTimelines.end(); it++) {
        WidgetTimeline &widgetTimeline = *it;
        if (widgetTimeline.flowState == flowState) {
            updateTimelineProperties(widgetTimeline, flowState->timelinePosition);
        }
    }

    for (auto childFlowState = flowState->firstChild; childFlowState; childFlowState = childFlowState->nextSibling) {
        doAnimateFlowState(childFlowState);
    }
}

void doAnimate() {
    if (g_currentScreen != -1) {
        auto flowState = eez::flow::getPageFlowState(eez::g_mainAssets, g_currentScreen);
        doAnimateFlowState(flowState);
    }
}

void setTimelinePosition(float timelinePosition) {
    for (auto it = widgetTimelines.begin(); it != widgetTimelines.end(); it++) {
        WidgetTimeline &widgetTimeline = *it;
        updateTimelineProperties(widgetTimeline, timelinePosition);
    }
}

void clearTimeline() {
    widgetTimelines.clear();
}

////////////////////////////////////////////////////////////////////////////////

struct UpdateTask {
    UpdateTaskType updateTaskType;
    lv_obj_t *obj;
    void *flow_state;
    unsigned component_index;
    unsigned property_index;
    void *subobj;
    int param;
};

static UpdateTask *g_updateTask;

#if LVGL_VERSION_MAJOR >= 9
#else
#define lv_event_get_target_obj lv_event_get_target
#endif

void flow_event_callback(lv_event_t *e) {
    FlowEventCallbackData *data = (FlowEventCallbackData *)e->user_data;
    e->user_data = (void *)data->user_data;
    flowPropagateValueLVGLEvent(data->flow_state, data->component_index, data->output_or_property_index, e);
}

void flow_event_textarea_text_changed_callback(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target_obj(e);
        if (!g_updateTask || g_updateTask->obj != ta) {
            FlowEventCallbackData *data = (FlowEventCallbackData *)e->user_data;
            const char *value = lv_textarea_get_text(ta);
            assignStringProperty(data->flow_state, data->component_index, data->output_or_property_index, value, "Failed to assign Text in Textarea widget");
        }
    }
}

void flow_event_checked_state_changed_callback(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target_obj(e);
        if (!g_updateTask || g_updateTask->obj != ta) {
            FlowEventCallbackData *data = (FlowEventCallbackData *)e->user_data;
            bool value = lv_obj_has_state(ta, LV_STATE_CHECKED);
            assignBooleanProperty(data->flow_state, data->component_index, data->output_or_property_index, value, "Failed to assign Checked state");
        }
    }
}

void flow_event_arc_value_changed_callback(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target_obj(e);
        if (!g_updateTask || g_updateTask->obj != ta) {
            FlowEventCallbackData *data = (FlowEventCallbackData *)e->user_data;
            int32_t value = lv_arc_get_value(ta);
            assignIntegerProperty(data->flow_state, data->component_index, data->output_or_property_index, value, "Failed to assign Value in Arc widget");
        }
    }
}

void flow_event_bar_value_changed_callback(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target_obj(e);
        if (!g_updateTask || g_updateTask->obj != ta) {
            FlowEventCallbackData *data = (FlowEventCallbackData *)e->user_data;
            int32_t value = lv_bar_get_value(ta);
            assignIntegerProperty(data->flow_state, data->component_index, data->output_or_property_index, value, "Failed to assign Value in Bar widget");
        }
    }
}

void flow_event_bar_value_start_changed_callback(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target_obj(e);
        if (!g_updateTask || g_updateTask->obj != ta) {
            FlowEventCallbackData *data = (FlowEventCallbackData *)e->user_data;
            int32_t value = lv_bar_get_start_value(ta);
            assignIntegerProperty(data->flow_state, data->component_index, data->output_or_property_index, value, "Failed to assign Value Start in Bar widget");
        }
    }
}

void flow_event_dropdown_selected_changed_callback(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target_obj(e);
        if (!g_updateTask || g_updateTask->obj != ta) {
            FlowEventCallbackData *data = (FlowEventCallbackData *)e->user_data;
            uint16_t selected = lv_dropdown_get_selected(ta);
            assignIntegerProperty(data->flow_state, data->component_index, data->output_or_property_index, selected, "Failed to assign Selected in Dropdown widget");
        }
    }
}

void flow_event_roller_selected_changed_callback(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target_obj(e);
        if (!g_updateTask || g_updateTask->obj != ta) {
            FlowEventCallbackData *data = (FlowEventCallbackData *)e->user_data;
            uint16_t selected = lv_roller_get_selected(ta);
            assignIntegerProperty(data->flow_state, data->component_index, data->output_or_property_index, selected, "Failed to assign Selected in Roller widget");
        }
    }
}

void flow_event_slider_value_changed_callback(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target_obj(e);
        if (!g_updateTask || g_updateTask->obj != ta) {
            FlowEventCallbackData *data = (FlowEventCallbackData *)e->user_data;
            int32_t value = lv_slider_get_value(ta);
            assignIntegerProperty(data->flow_state, data->component_index, data->output_or_property_index, value, "Failed to assign Value in Slider widget");
        }
    }
}

void flow_event_slider_value_left_changed_callback(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target_obj(e);
        if (!g_updateTask || g_updateTask->obj != ta) {
            FlowEventCallbackData *data = (FlowEventCallbackData *)e->user_data;
            int32_t value = lv_slider_get_left_value(ta);
            assignIntegerProperty(data->flow_state, data->component_index, data->output_or_property_index, value, "Failed to assign Value Left in Slider widget");
        }
    }
}

void flow_event_spinbox_value_changed_callback(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target_obj(e);
        if (!g_updateTask || g_updateTask->obj != ta) {
            FlowEventCallbackData *data = (FlowEventCallbackData *)e->user_data;
            int32_t value = lv_spinbox_get_value(ta);
            assignIntegerProperty(data->flow_state, data->component_index, data->output_or_property_index, value, "Failed to assign Value in Spinbox widget");
        }
    }
}

void flow_event_spinbox_step_changed_callback(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target_obj(e);
        if (!g_updateTask || g_updateTask->obj != ta) {
            FlowEventCallbackData *data = (FlowEventCallbackData *)e->user_data;
            int32_t value = lv_spinbox_get_step(ta);
            assignIntegerProperty(data->flow_state, data->component_index, data->output_or_property_index, value, "Failed to assign Step in Spinbox widget");
        }
    }
}

void flow_event_spinbox_min_changed_callback(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target_obj(e);
        if (!g_updateTask || g_updateTask->obj != ta) {
            FlowEventCallbackData *data = (FlowEventCallbackData *)e->user_data;
            lv_spinbox_t* spinbox = (lv_spinbox_t *) ta;
            int32_t value = spinbox->range_min;
            assignIntegerProperty(data->flow_state, data->component_index, data->output_or_property_index, value, "Failed to assign Min in Spinbox widget");
        }
    }
}

void flow_event_spinbox_max_changed_callback(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target_obj(e);
        if (!g_updateTask || g_updateTask->obj != ta) {
            FlowEventCallbackData *data = (FlowEventCallbackData *)e->user_data;
            lv_spinbox_t* spinbox = (lv_spinbox_t *) ta;
            int32_t value = spinbox->range_max;
            assignIntegerProperty(data->flow_state, data->component_index, data->output_or_property_index, value, "Failed to assign Max in Spinbox widget");
        }
    }
}

void flow_event_spinbox_digit_counter_changed_callback(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target_obj(e);
        if (!g_updateTask || g_updateTask->obj != ta) {
            FlowEventCallbackData *data = (FlowEventCallbackData *)e->user_data;
            lv_spinbox_t* spinbox = (lv_spinbox_t *) ta;
            int32_t value = spinbox->digit_count;
            assignIntegerProperty(data->flow_state, data->component_index, data->output_or_property_index, value, "Failed to assign Digit Counter in Spinbox widget");
        }
    }
}

void flow_event_spinbox_separator_position_changed_callback(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target_obj(e);
        if (!g_updateTask || g_updateTask->obj != ta) {
            FlowEventCallbackData *data = (FlowEventCallbackData *)e->user_data;
            lv_spinbox_t* spinbox = (lv_spinbox_t *) ta;
            int32_t maxval = spinbox->digit_count - 1;
            int32_t value = spinbox->dec_point_pos;
            if (value > 0 && value <= maxval) {
                assignIntegerProperty(data->flow_state, data->component_index, data->output_or_property_index, value, "Failed to assign Separator Position in Spinbox widget");
            }
        }
    }
}


void flow_event_checked_callback(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t *ta = lv_event_get_target_obj(e);
    if (event == LV_EVENT_VALUE_CHANGED && lv_obj_has_state(ta, LV_STATE_CHECKED)) {
        flow_event_callback(e);
    }
}

void flow_event_unchecked_callback(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    lv_obj_t *ta = lv_event_get_target_obj(e);
    if (event == LV_EVENT_VALUE_CHANGED && !lv_obj_has_state(ta, LV_STATE_CHECKED)) {
        flow_event_callback(e);
    }
}
#if LVGL_VERSION_MAJOR >= 9
void flow_event_meter_tick_label_event_callback(lv_event_t *e) {
    // TODO LVGL 9.0
}
#else
void flow_event_meter_tick_label_event_callback(lv_event_t *e) {
    lv_obj_draw_part_dsc_t * draw_part_dsc = lv_event_get_draw_part_dsc(e);

    // Be sure it's drawing meter related parts
    if (draw_part_dsc->class_p != &lv_meter_class) return;

    // Be sure it's drawing the ticks
    if (draw_part_dsc->type != LV_METER_DRAW_PART_TICK) return;

    g_eezFlowLvlgMeterTickIndex = draw_part_dsc->id;
    FlowEventCallbackData *data = (FlowEventCallbackData *)e->user_data;
    const char *temp = evalTextProperty(data->flow_state, data->component_index, data->output_or_property_index, "Failed to evalute scale label in Meter widget");
    if (temp) {
        static char label[32];
        strncpy(label, temp, sizeof(label));
        label[sizeof(label) - 1] = 0;
        draw_part_dsc->text = label;
        draw_part_dsc->text_length = sizeof(label);
    }
}
#endif

void flow_event_callback_delete_user_data(lv_event_t *e) {
#if LVGL_VERSION_MAJOR >= 9
    lv_free(e->user_data);
#else
    lv_mem_free(e->user_data);
#endif
}

////////////////////////////////////////////////////////////////////////////////

std::vector<UpdateTask> updateTasks;

void addUpdateTask(UpdateTaskType updateTaskType, lv_obj_t *obj, void *flow_state, unsigned component_index, unsigned property_index, void *subobj, int param) {
    UpdateTask updateTask;
    updateTask.updateTaskType = updateTaskType;
    updateTask.obj = obj;
    updateTask.flow_state = flow_state;
    updateTask.component_index = component_index;
    updateTask.property_index = property_index;
    updateTask.subobj = subobj;
    updateTask.param = param;
    updateTasks.push_back(updateTask);
}

void doUpdateTasks() {
    for (auto it = updateTasks.begin(); it != updateTasks.end(); it++) {
        UpdateTask &updateTask = *it;
        g_updateTask = &updateTask;
        if (updateTask.updateTaskType == UPDATE_TASK_TYPE_LABEL_TEXT) {
            const char *new_val = evalTextProperty(updateTask.flow_state, updateTask.component_index, updateTask.property_index, "Failed to evaluate Text in Label widget");
            const char *cur_val = lv_label_get_text(updateTask.obj);
            if (strcmp(new_val, cur_val) != 0) lv_label_set_text(updateTask.obj, new_val ? new_val : "");
        } else if (updateTask.updateTaskType == UPDATE_TASK_TYPE_TEXTAREA_TEXT) {
            const char *new_val = evalTextProperty(updateTask.flow_state, updateTask.component_index, updateTask.property_index, "Failed to evaluate Text in Textarea widget");
            const char *cur_val = lv_textarea_get_text(updateTask.obj);
            if (strcmp(new_val, cur_val) != 0) lv_textarea_set_text(updateTask.obj, new_val);
        } else if (updateTask.updateTaskType == UPDATE_TASK_TYPE_DROPDOWN_OPTIONS) {
            const char *new_val = evalStringArrayPropertyAndJoin(updateTask.flow_state, updateTask.component_index, updateTask.property_index, "Failed to evaluate Selected in Dropdown widget", "\n");
            const char *cur_val = lv_dropdown_get_options(updateTask.obj);
            if (strcmp(new_val, cur_val) != 0) lv_dropdown_set_options(updateTask.obj, new_val);
        } else if (updateTask.updateTaskType == UPDATE_TASK_TYPE_DROPDOWN_SELECTED) {
            uint16_t new_val = (uint16_t)evalIntegerProperty(updateTask.flow_state, updateTask.component_index, updateTask.property_index, "Failed to evaluate Selected in Dropdown widget");
            uint16_t cur_val = lv_dropdown_get_selected(updateTask.obj);
            if (new_val != cur_val) lv_dropdown_set_selected(updateTask.obj, new_val);
        } else if (updateTask.updateTaskType == UPDATE_TASK_TYPE_ROLLER_OPTIONS) {
            const char *new_val = evalStringArrayPropertyAndJoin(updateTask.flow_state, updateTask.component_index, updateTask.property_index, "Failed to evaluate Selected in Dropdown widget", "\n");
            const char *cur_val = lv_roller_get_options(updateTask.obj);
            if (compareRollerOptions((lv_roller_t *)updateTask.obj, new_val, cur_val, (lv_roller_mode_t)updateTask.param)) {
                lv_roller_set_options(updateTask.obj, new_val, (lv_roller_mode_t)updateTask.param);
            }
        } else if (updateTask.updateTaskType == UPDATE_TASK_TYPE_ROLLER_SELECTED) {
            uint16_t new_val = (uint16_t)evalIntegerProperty(updateTask.flow_state, updateTask.component_index, updateTask.property_index, "Failed to evaluate Selected in Roller widget");
            uint16_t cur_val = lv_roller_get_selected(updateTask.obj);
            if (new_val != cur_val) lv_roller_set_selected(updateTask.obj, new_val, LV_ANIM_OFF);
        } else if (updateTask.updateTaskType == UPDATE_TASK_TYPE_SLIDER_VALUE) {
            int32_t new_val = evalIntegerProperty(updateTask.flow_state, updateTask.component_index, updateTask.property_index, "Failed to evaluate Value in Slider widget");
            int32_t cur_val = lv_slider_get_value(updateTask.obj);
            if (new_val != cur_val) lv_slider_set_value(updateTask.obj, new_val, updateTask.param ? LV_ANIM_ON : LV_ANIM_OFF);
        } else if (updateTask.updateTaskType == UPDATE_TASK_TYPE_SLIDER_VALUE_LEFT) {
            int32_t new_val = evalIntegerProperty(updateTask.flow_state, updateTask.component_index, updateTask.property_index, "Failed to evaluate Value Left in Slider widget");
            int32_t cur_val = lv_slider_get_left_value(updateTask.obj);
            if (new_val != cur_val) lv_slider_set_left_value(updateTask.obj, new_val, updateTask.param ? LV_ANIM_ON : LV_ANIM_OFF);
        } else if (updateTask.updateTaskType == UPDATE_TASK_TYPE_ARC_RANGE_MIN) {
            int32_t new_val = evalIntegerProperty(updateTask.flow_state, updateTask.component_index, updateTask.property_index, "Failed to evaluate Range min in Arc widget");
            int32_t cur_val = lv_arc_get_min_value(updateTask.obj);
            if (new_val != cur_val) {
                auto max = lv_arc_get_max_value(updateTask.obj);
                if (new_val < max) {
                    lv_arc_set_range(updateTask.obj, new_val, max);
                }
            }
        } else if (updateTask.updateTaskType == UPDATE_TASK_TYPE_ARC_RANGE_MAX) {
            int32_t new_val = evalIntegerProperty(updateTask.flow_state, updateTask.component_index, updateTask.property_index, "Failed to evaluate Range max in Arc widget");
            int32_t cur_val = lv_arc_get_max_value(updateTask.obj);
            if (new_val != cur_val) {
                auto min = lv_arc_get_min_value(updateTask.obj);
                if (new_val > min) {
                    lv_arc_set_range(updateTask.obj, min, new_val);
                }
            }
        } else if (updateTask.updateTaskType == UPDATE_TASK_TYPE_ARC_VALUE) {
            int32_t new_val = evalIntegerProperty(updateTask.flow_state, updateTask.component_index, updateTask.property_index, "Failed to evaluate Value in Arc widget");
            int32_t cur_val = lv_arc_get_value(updateTask.obj);
            if (new_val != cur_val) lv_arc_set_value(updateTask.obj, new_val);
        } else if (updateTask.updateTaskType == UPDATE_TASK_TYPE_BAR_VALUE) {
            int32_t new_val = evalIntegerProperty(updateTask.flow_state, updateTask.component_index, updateTask.property_index, "Failed to evaluate Value in Bar widget");
            int32_t cur_val = lv_bar_get_value(updateTask.obj);
            if (new_val != cur_val) lv_bar_set_value(updateTask.obj, new_val, updateTask.param ? LV_ANIM_ON : LV_ANIM_OFF);
        } else if (updateTask.updateTaskType == UPDATE_TASK_TYPE_BAR_VALUE_START) {
            int32_t new_val = evalIntegerProperty(updateTask.flow_state, updateTask.component_index, updateTask.property_index, "Failed to evaluate Value Start in Bar widget");
            int32_t cur_val = lv_bar_get_start_value(updateTask.obj);
            if (new_val != cur_val) lv_bar_set_start_value(updateTask.obj, new_val, updateTask.param ? LV_ANIM_ON : LV_ANIM_OFF);
        } else if (updateTask.updateTaskType == UPDATE_TASK_TYPE_CHECKED_STATE) {
            bool new_val = evalBooleanProperty(updateTask.flow_state, updateTask.component_index, updateTask.property_index, "Failed to evaluate Checked state");
            bool cur_val = lv_obj_has_state(updateTask.obj, LV_STATE_CHECKED);
            if (new_val != cur_val) {
                if (new_val) lv_obj_add_state(updateTask.obj, LV_STATE_CHECKED);
                else lv_obj_clear_state(updateTask.obj, LV_STATE_CHECKED);
            }
        } else if (updateTask.updateTaskType == UPDATE_TASK_TYPE_DISABLED_STATE) {
            bool new_val = evalBooleanProperty(updateTask.flow_state, updateTask.component_index, updateTask.property_index, "Failed to evaluate Disabled state");
            bool cur_val = lv_obj_has_state(updateTask.obj, LV_STATE_DISABLED);
            if (new_val != cur_val) {
                if (new_val) lv_obj_add_state(updateTask.obj, LV_STATE_DISABLED);
                else lv_obj_clear_state(updateTask.obj, LV_STATE_DISABLED);
            }
        } else if (updateTask.updateTaskType == UPDATE_TASK_TYPE_HIDDEN_FLAG) {
            bool new_val = evalBooleanProperty(updateTask.flow_state, updateTask.component_index, updateTask.property_index, "Failed to evaluate Hidden flag");
            bool cur_val = lv_obj_has_flag(updateTask.obj, LV_OBJ_FLAG_HIDDEN);
            if (new_val != cur_val) {
                if (new_val) lv_obj_add_flag(updateTask.obj, LV_OBJ_FLAG_HIDDEN);
                else lv_obj_clear_flag(updateTask.obj, LV_OBJ_FLAG_HIDDEN);
            }
        } else if (updateTask.updateTaskType == UPDATE_TASK_TYPE_CLICKABLE_FLAG) {
            bool new_val = evalBooleanProperty(updateTask.flow_state, updateTask.component_index, updateTask.property_index, "Failed to evaluate Clickable flag");
            bool cur_val = lv_obj_has_flag(updateTask.obj, LV_OBJ_FLAG_CLICKABLE);
            if (new_val != cur_val) {
                if (new_val) lv_obj_add_flag(updateTask.obj, LV_OBJ_FLAG_CLICKABLE);
                else lv_obj_clear_flag(updateTask.obj, LV_OBJ_FLAG_CLICKABLE);
            }
        } else if (updateTask.updateTaskType == UPDATE_TASK_TYPE_METER_INDICATOR_VALUE) {
#if LVGL_VERSION_MAJOR >= 9
    // TODO LVGL 9.0
#else
            int32_t new_val = evalIntegerProperty(updateTask.flow_state, updateTask.component_index, updateTask.property_index, "Failed to evaluate Indicator Value in Meter widget");
            lv_meter_indicator_t *indicator = (lv_meter_indicator_t *)updateTask.subobj;
            int32_t cur_val = indicator->start_value;
            if (new_val != cur_val) lv_meter_set_indicator_value(updateTask.obj, indicator, new_val);
#endif
        } else if (updateTask.updateTaskType == UPDATE_TASK_TYPE_METER_INDICATOR_START_VALUE) {
#if LVGL_VERSION_MAJOR >= 9
    // TODO LVGL 9.0
#else
            int32_t new_val = evalIntegerProperty(updateTask.flow_state, updateTask.component_index, updateTask.property_index, "Failed to evaluate Indicator Start Value in Meter widget");
            lv_meter_indicator_t *indicator = (lv_meter_indicator_t *)updateTask.subobj;
            int32_t cur_val = indicator->start_value;
            if (new_val != cur_val) lv_meter_set_indicator_start_value(updateTask.obj, indicator, new_val);
#endif
        } else if (updateTask.updateTaskType == UPDATE_TASK_TYPE_METER_INDICATOR_END_VALUE) {
#if LVGL_VERSION_MAJOR >= 9
    // TODO LVGL 9.0
#else
            int32_t new_val = evalIntegerProperty(updateTask.flow_state, updateTask.component_index, updateTask.property_index, "Failed to evaluate Indicator End Value in Meter widget");
            lv_meter_indicator_t *indicator = (lv_meter_indicator_t *)updateTask.subobj;
            int32_t cur_val = indicator->end_value;
            if (new_val != cur_val) lv_meter_set_indicator_end_value(updateTask.obj, indicator, new_val);
#endif
        } else if (updateTask.updateTaskType == UPDATE_TASK_TYPE_TAB_NAME) {
            const char *new_val = evalTextProperty(updateTask.flow_state, updateTask.component_index, updateTask.property_index, "Failed to evaluate Tab name in Tab widget");

            uint32_t tab_id = updateTask.param;

            lv_obj_t *tabview = lv_obj_get_parent(updateTask.obj);
            if (!lv_obj_check_type(tabview, &lv_tabview_class)) {
                tabview = lv_obj_get_parent(tabview);
            }

            if (lv_obj_check_type(tabview, &lv_tabview_class)) {
#if LVGL_VERSION_MAJOR >= 9
                lv_obj_t *tab_bar = lv_tabview_get_tab_bar(tabview);
                lv_obj_t *button = lv_obj_get_child_by_type(tab_bar, tab_id, &lv_button_class);
                lv_obj_t *label = lv_obj_get_child_by_type(button, 0, &lv_label_class);
                const char *cur_val = lv_label_get_text(label);
#else
                const char *cur_val = ((lv_tabview_t *)tabview)->map[tab_id * (((lv_tabview_t *)tabview)->tab_pos & LV_DIR_HOR ? 2 : 1)];
#endif

                if (strcmp(new_val, cur_val) != 0) lv_tabview_rename_tab(tabview, (uint32_t)updateTask.param, new_val ? new_val : "");
            }
        } else if (updateTask.updateTaskType == UPDATE_TASK_TYPE_LED_COLOR) {
            uint32_t new_val = evalIntegerProperty(updateTask.flow_state, updateTask.component_index, updateTask.property_index, "Failed to evaluate Color in Led widget");
#if LVGL_VERSION_MAJOR >= 9
            uint32_t cur_val = lv_color_to_u32(((lv_led_t *)updateTask.obj)->color);
#else
            uint32_t cur_val = lv_color_to32(((lv_led_t *)updateTask.obj)->color);
#endif
            if (new_val != cur_val) lv_led_set_color(updateTask.obj, lv_color_hex(new_val));
        } else if (updateTask.updateTaskType == UPDATE_TASK_TYPE_LED_BRIGHTNESS) {
            uint32_t new_val = evalIntegerProperty(updateTask.flow_state, updateTask.component_index, updateTask.property_index, "Failed to evaluate Brightness in Led widget");
            if (new_val < 0) {
                new_val = 0;
            } else if (new_val > 255) {
                new_val = 255;
            }
            uint32_t cur_val = lv_led_get_brightness(updateTask.obj);
            if (new_val != cur_val) lv_led_set_brightness(updateTask.obj, new_val);
        } else if (updateTask.updateTaskType == UPDATE_TASK_TYPE_SPINBOX_VALUE) {
            int32_t new_val = evalIntegerProperty(updateTask.flow_state, updateTask.component_index, updateTask.property_index, "Failed to evaluate Value in Spinbox widget");
            int32_t cur_val = lv_spinbox_get_value(updateTask.obj);
            lv_spinbox_t * spinbox = (lv_spinbox_t *) updateTask.obj;
            int32_t min_val = spinbox->range_min;
            int32_t max_val = spinbox->range_max;
            if (new_val != cur_val && new_val >= min_val && new_val <= max_val ) {
                lv_spinbox_set_value(updateTask.obj, new_val);
            }
        } else if (updateTask.updateTaskType == UPDATE_TASK_TYPE_SPINBOX_STEP) {
            int32_t new_val = evalIntegerProperty(updateTask.flow_state, updateTask.component_index, updateTask.property_index, "Failed to evaluate Step in Spinbox widget");
            int32_t cur_val = lv_spinbox_get_step(updateTask.obj);
            if (new_val != cur_val) lv_spinbox_set_step(updateTask.obj, new_val);
        } else if (updateTask.updateTaskType == UPDATE_TASK_TYPE_SPINBOX_MIN) {
            int32_t new_val = evalIntegerProperty(updateTask.flow_state, updateTask.component_index, updateTask.property_index, "Failed to evaluate Min in Spinbox widget");
            lv_spinbox_t * spinbox = (lv_spinbox_t *) updateTask.obj;
            int32_t cur_val = spinbox->range_min;
            int32_t max_val = spinbox->range_max;
            if (new_val != cur_val) lv_spinbox_set_range(updateTask.obj, new_val, max_val);
        } else if (updateTask.updateTaskType == UPDATE_TASK_TYPE_SPINBOX_MAX) {
            int32_t new_val = evalIntegerProperty(updateTask.flow_state, updateTask.component_index, updateTask.property_index, "Failed to evaluate Max in Spinbox widget");
            lv_spinbox_t * spinbox = (lv_spinbox_t *) updateTask.obj;
            int32_t cur_val = spinbox->range_max;
            int32_t min_val = spinbox->range_min;
            if (new_val != cur_val) lv_spinbox_set_range(updateTask.obj, min_val, new_val);
        } else if (updateTask.updateTaskType == UPDATE_TASK_TYPE_SPINBOX_COUNTER) {
            int32_t new_val = evalIntegerProperty(updateTask.flow_state, updateTask.component_index, updateTask.property_index, "Failed to evaluate Digit Counter in Spinbox widget");
            lv_spinbox_t * spinbox = (lv_spinbox_t *) updateTask.obj;
            int32_t cur_val = spinbox->digit_count;
            int32_t dec_point = spinbox->dec_point_pos;
            if (new_val != cur_val && new_val >= 1) {
                if (dec_point <= new_val) {
                    dec_point = 0;
                } 
                lv_spinbox_set_digit_format(updateTask.obj, new_val, dec_point);
            }
        } else if (updateTask.updateTaskType == UPDATE_TASK_TYPE_SPINBOX_SEPARATOR) {
            int32_t new_val = evalIntegerProperty(updateTask.flow_state, updateTask.component_index, updateTask.property_index, "Failed to evaluate Max in Spinbox widget");
            lv_spinbox_t * spinbox = (lv_spinbox_t *) updateTask.obj;
            int32_t cur_val = spinbox->dec_point_pos;
            int32_t digits = spinbox->digit_count;
            if (new_val != cur_val && new_val >= 0 && new_val < digits) {
                lv_spinbox_set_digit_format(updateTask.obj, digits, new_val);
            }
        }
        g_updateTask = nullptr;
    }

}

////////////////////////////////////////////////////////////////////////////////

void startToDebuggerMessage() {
    EM_ASM({
        startToDebuggerMessage($0);
    }, eez::flow::g_wasmModuleId);
}

static char g_debuggerBuffer[1024 * 1024];
static uint32_t g_debuggerBufferIndex = 0;

void writeDebuggerBuffer(const char *buffer, uint32_t length) {
    if (g_debuggerBufferIndex + length > sizeof(g_debuggerBuffer)) {
        EM_ASM({
            writeDebuggerBuffer($0, new Uint8Array(Module.HEAPU8.buffer, $1, $2));
        }, eez::flow::g_wasmModuleId, g_debuggerBuffer, g_debuggerBufferIndex);
        g_debuggerBufferIndex = 0;
    } else {
        memcpy(g_debuggerBuffer + g_debuggerBufferIndex, buffer, length);
        g_debuggerBufferIndex += length;
    }
}

void finishToDebuggerMessage() {
    if (g_debuggerBufferIndex > 0) {
        EM_ASM({
            writeDebuggerBuffer($0, new Uint8Array(Module.HEAPU8.buffer, $1, $2));
        }, eez::flow::g_wasmModuleId, g_debuggerBuffer, g_debuggerBufferIndex);
        g_debuggerBufferIndex = 0;
    }

    EM_ASM({
        finishToDebuggerMessage($0);
    }, eez::flow::g_wasmModuleId);
}

void replacePageHook(int16_t pageId, uint32_t animType, uint32_t speed, uint32_t delay) {
    screenLoad_animType = animType;
    screenLoad_speed = speed;
    screenLoad_delay = delay;
    eez::flow::onPageChanged(g_currentScreen + 1, pageId);
    g_currentScreen = pageId - 1;
}

EM_PORT_API(void) stopScript() {
    eez::flow::stop();
}

EM_PORT_API(void) onMessageFromDebugger(char *messageData, uint32_t messageDataSize) {
    eez::flow::processDebuggerInput(messageData, messageDataSize);
}

EM_PORT_API(void *) lvglGetFlowState(void *flowState, unsigned userWidgetComponentIndexOrPageIndex) {
    return getFlowState(flowState, userWidgetComponentIndexOrPageIndex);
}

EM_PORT_API(void) setDebuggerMessageSubsciptionFilter(uint32_t filter) {
    eez::flow::setDebuggerMessageSubsciptionFilter(filter);
}

////////////////////////////////////////////////////////////////////////////////

static std::map<int, lv_obj_t *> indexToObject;

void setObjectIndex(lv_obj_t *obj, int32_t index) {
    indexToObject.insert(std::make_pair(index, obj));
}

static lv_obj_t *getLvglObjectFromIndex(int32_t index) {
    auto it = indexToObject.find(index);
    if (it == indexToObject.end()) {
        return nullptr;
    }
    return it->second;
}

////////////////////////////////////////////////////////////////////////////////

static const void *getLvglImageByName(const char *name) {
    return (const void *)EM_ASM_INT({
        return getLvglImageByName($0, UTF8ToString($1));
    }, eez::flow::g_wasmModuleId, name);
}

////////////////////////////////////////////////////////////////////////////////

static void lvglObjAddStyle(lv_obj_t *obj, int32_t styleIndex) {
    return EM_ASM({
        lvglObjAddStyle($0, $1, $2);
    }, eez::flow::g_wasmModuleId, obj, styleIndex);
}

static void lvglObjRemoveStyle(lv_obj_t *obj, int32_t styleIndex) {
    return EM_ASM({
        lvglObjRemoveStyle($0, $1, $2);
    }, eez::flow::g_wasmModuleId, obj, styleIndex);
}

////////////////////////////////////////////////////////////////////////////////

extern "C" void flowInit(uint32_t wasmModuleId, uint32_t debuggerMessageSubsciptionFilter, uint8_t *assets, uint32_t assetsSize, bool darkTheme, uint32_t timeZone) {
    lv_disp_t * dispp = lv_disp_get_default();
    lv_theme_t * theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), darkTheme, LV_FONT_DEFAULT);
    //DISPLAY_WIDTH = eez::g_mainAssets->settings->displayWidth;
    //DISPLAY_HEIGHT = eez::g_mainAssets->settings->displayHeight;

    eez::flow::g_wasmModuleId = wasmModuleId;

    eez::flow::date::g_timeZone = timeZone;

    eez::initAssetsMemory();
    eez::loadMainAssets(assets, assetsSize);
    eez::initOtherMemory();
    eez::initAllocHeap(eez::ALLOC_BUFFER, eez::ALLOC_BUFFER_SIZE);

    eez::flow::startToDebuggerMessageHook = startToDebuggerMessage;
    eez::flow::writeDebuggerBufferHook = writeDebuggerBuffer;
    eez::flow::finishToDebuggerMessageHook = finishToDebuggerMessage;
    eez::flow::replacePageHook = replacePageHook;
    eez::flow::stopScriptHook = stopScript;
    eez::flow::getLvglObjectFromIndexHook = getLvglObjectFromIndex;
    eez::flow::getLvglImageByNameHook = getLvglImageByName;
    eez::flow::lvglObjAddStyleHook = lvglObjAddStyle;
    eez::flow::lvglObjRemoveStyleHook = lvglObjRemoveStyle;

    eez::flow::setDebuggerMessageSubsciptionFilter(debuggerMessageSubsciptionFilter);
    eez::flow::onDebuggerClientConnected();

    eez::flow::start(eez::g_mainAssets);

    g_currentScreen = 0;
}

extern "C" bool flowTick() {
    if (eez::flow::isFlowStopped()) {
        return false;
    }

    eez::flow::tick();

    if (eez::flow::isFlowStopped()) {
        return false;
    }

    doAnimate();

    doUpdateTasks();

    return true;
}

void flowOnPageLoadedStudio(unsigned pageIndex) {
    if (g_currentScreen == -1) {
        g_currentScreen = pageIndex;
    }
    eez::flow::getPageFlowState(eez::g_mainAssets, pageIndex);
}

native_var_t native_vars[] = {
    { NATIVE_VAR_TYPE_NONE, 0, 0 },
};
