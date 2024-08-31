import React from "react";
import { makeObservable } from "mobx";

import {
    IMessage,
    MessageType,
    makeDerivedClassInfo
} from "project-editor/core/object";

import { ProjectType } from "project-editor/project/project";

import { LVGLPageRuntime } from "project-editor/lvgl/page-runtime";
import type { LVGLBuild } from "project-editor/lvgl/build";
import { LVGLParts } from "project-editor/lvgl/lvgl-constants";

import { LVGLTabviewWidget, LVGLTabWidget, LVGLWidget } from "./internal";
import { getTabview } from "../widget-common";
import { getProjectStore, Message } from "project-editor/store";
import { getLvglParts } from "../lvgl-versions";
import { Rect } from "eez-studio-shared/geometry";
import { AutoSize } from "project-editor/flow/component";
import { IResizeHandler } from "project-editor/flow/flow-interfaces";
import {
    bg_opa_property_info,
    border_width_property_info,
    pad_bottom_property_info,
    pad_left_property_info,
    pad_right_property_info,
    pad_top_property_info,
    radius_property_info
} from "../style-catalog";

////////////////////////////////////////////////////////////////////////////////

export class LVGLContainerWidget extends LVGLWidget {
    static classInfo = makeDerivedClassInfo(LVGLWidget.classInfo, {
        enabledInComponentPalette: (projectType: ProjectType) =>
            projectType === ProjectType.LVGL,

        label: (widget: LVGLTabWidget) => {
            const tabview = getTabview(widget);
            if (tabview) {
                if (tabview.children.indexOf(widget) == 0) {
                    return "Bar";
                } else if (tabview.children.indexOf(widget) == 1) {
                    return "Content";
                }
            }

            return LVGLWidget.classInfo.label!(widget);
        },

        componentPaletteGroupName: "!1Basic",

        properties: [],

        defaultValue: {
            left: 0,
            top: 0,
            width: 100,
            height: 50,
            clickableFlag: true
        },

        check: (widget: LVGLTabviewWidget, messages: IMessage[]) => {
            const tabview = getTabview(widget);
            if (tabview) {
                if (tabview.children.indexOf(widget) == 1) {
                    for (let i = 0; i < widget.children.length; i++) {
                        const childWidget = widget.children[i];
                        if (!(childWidget instanceof LVGLTabWidget)) {
                            messages.push(
                                new Message(
                                    MessageType.ERROR,
                                    `Tab should be child of Content container`,
                                    childWidget
                                )
                            );
                        }
                    }
                }
            }
        },

        icon: (
            <svg
                strokeWidth="2"
                stroke="currentColor"
                fill="none"
                strokeLinecap="round"
                strokeLinejoin="round"
                viewBox="0 0 24 24"
            >
                <path d="M0 0h24v24H0z" stroke="none" />
                <rect x="3" y="5" width="18" height="14" rx="2" />
            </svg>
        ),

        lvgl: {
            parts: (widget: LVGLWidget) =>
                Object.keys(getLvglParts(widget)) as LVGLParts[],
            defaultFlags:
                "CLICKABLE|CLICK_FOCUSABLE|GESTURE_BUBBLE|PRESS_LOCK|SCROLLABLE|SCROLL_CHAIN_HOR|SCROLL_CHAIN_VER|SCROLL_ELASTIC|SCROLL_MOMENTUM|SCROLL_WITH_ARROW|SNAPPABLE",

            oldInitFlags:
                "PRESS_LOCK|CLICK_FOCUSABLE|GESTURE_BUBBLE|SNAPPABLE|SCROLL_ELASTIC|SCROLL_MOMENTUM|SCROLL_CHAIN",
            oldDefaultFlags:
                "CLICKABLE|PRESS_LOCK|CLICK_FOCUSABLE|GESTURE_BUBBLE|SNAPPABLE|SCROLLABLE|SCROLL_ELASTIC|SCROLL_MOMENTUM|SCROLL_CHAIN"
        },

        setRect: (widget: LVGLContainerWidget, value: Partial<Rect>) => {
            const tabview = getTabview(widget);
            if (tabview) {
                if (tabview.children.indexOf(widget) == 0) {
                    if (
                        (tabview.tabviewPosition == "TOP" ||
                            tabview.tabviewPosition == "BOTTOM") &&
                        value.height != undefined
                    ) {
                        const projectStore = getProjectStore(widget);
                        projectStore.updateObject(tabview, {
                            tabviewSize: value.height
                        });
                    } else if (
                        (tabview.tabviewPosition == "LEFT" ||
                            tabview.tabviewPosition == "RIGHT") &&
                        value.width != undefined
                    ) {
                        const projectStore = getProjectStore(widget);
                        projectStore.updateObject(tabview, {
                            tabviewSize: value.width
                        });
                    }
                }
            } else {
                LVGLWidget.classInfo.setRect!(widget, value);
            }
        }
    });

    override makeEditable() {
        super.makeEditable();

        makeObservable(this, {});
    }

    override get autoSize(): AutoSize {
        const tabview = getTabview(this);
        if (tabview) {
            if (
                tabview.children.indexOf(this) == 0 ||
                tabview.children.indexOf(this) == 1
            ) {
                return "both";
            }
        }

        return super.autoSize;
    }

    override getResizeHandlers(): IResizeHandler[] | undefined | false {
        const tabview = getTabview(this);
        if (tabview && tabview.children.indexOf(this) == 0) {
            if (
                tabview.tabviewPosition == "TOP" ||
                tabview.tabviewPosition == "BOTTOM"
            ) {
                return [
                    {
                        x: 50,
                        y: 0,
                        type: "n-resize"
                    },
                    {
                        x: 50,
                        y: 100,
                        type: "s-resize"
                    }
                ];
            } else if (
                tabview.tabviewPosition == "LEFT" ||
                tabview.tabviewPosition == "RIGHT"
            ) {
                return [
                    {
                        x: 0,
                        y: 50,
                        type: "w-resize"
                    },
                    {
                        x: 100,
                        y: 50,
                        type: "e-resize"
                    }
                ];
            }
        }

        return super.getResizeHandlers();
    }

    override lvglCreateObj(
        runtime: LVGLPageRuntime,
        parentObj: number
    ): number {
        const tabview = getTabview(this);
        if (tabview) {
            if (tabview.children.indexOf(this) == 0) {
                return runtime.wasm._lvglTabviewGetTabBar(
                    parentObj,
                    runtime.getWidgetIndex(this)
                );
            }

            if (tabview.children.indexOf(this) == 1) {
                return runtime.wasm._lvglTabviewGetTabContent(
                    parentObj,
                    runtime.getWidgetIndex(this)
                );
            }
        }

        const rect = this.getLvglCreateRect();

        return runtime.wasm._lvglCreateContainer(
            parentObj,
            runtime.getWidgetIndex(this),

            rect.left,
            rect.top,
            rect.width,
            rect.height
        );
    }

    override lvglBuildObj(build: LVGLBuild) {
        const tabview = getTabview(this);
        if (tabview) {
            if (tabview.children.indexOf(this) == 0) {
                if (build.isV9) {
                    build.line(
                        `lv_obj_t *obj = lv_tabview_get_tab_bar(parent_obj);`
                    );
                } else {
                    build.line(
                        `lv_obj_t *obj = lv_tabview_get_tab_btns(parent_obj);`
                    );
                }
                return;
            }

            if (tabview.children.indexOf(this) == 1) {
                build.line(
                    `lv_obj_t *obj = lv_tabview_get_content(parent_obj);`
                );
                return;
            }
        }

        build.line(`lv_obj_t *obj = lv_obj_create(parent_obj);`);
    }

    override lvglBuildSpecific(build: LVGLBuild) {
        this.buildStyleIfNotDefined(build, pad_left_property_info);
        this.buildStyleIfNotDefined(build, pad_top_property_info);
        this.buildStyleIfNotDefined(build, pad_right_property_info);
        this.buildStyleIfNotDefined(build, pad_bottom_property_info);
        this.buildStyleIfNotDefined(build, bg_opa_property_info);
        this.buildStyleIfNotDefined(build, border_width_property_info);
        this.buildStyleIfNotDefined(build, radius_property_info);
    }
}
