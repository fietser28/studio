import React from "react";
import { makeObservable, observable } from "mobx";

import { makeDerivedClassInfo, PropertyType } from "project-editor/core/object";

import { type Project, ProjectType } from "project-editor/project/project";

import { LVGLPageRuntime } from "project-editor/lvgl/page-runtime";
import type { LVGLBuild } from "project-editor/lvgl/build";

import { LVGLWidget } from "./internal";
import { specificGroup } from "project-editor/ui-components/PropertyGrid/groups";
import {
    expressionPropertyBuildEventHandlerSpecific,
    expressionPropertyBuildTickSpecific,
    LVGLPropertyType,
    makeLvglExpressionProperty
} from "../expression-property";
import {
    getExpressionPropertyData,
    getFlowStateAddressIndex,
    lvglAddObjectFlowCallback
} from "../widget-common";
import {
    LV_DIR_LEFT,
    LV_DIR_RIGHT,
    LV_EVENT_SPINBOX_STEP_CHANGED,
    LV_EVENT_SPINBOX_VALUE_CHANGED,
    LV_EVENT_SPINBOX_MIN_CHANGED,
    LV_EVENT_SPINBOX_MAX_CHANGED,
    LV_EVENT_SPINBOX_DIGIT_COUNTER_CHANGED,
    LV_EVENT_SPINBOX_SEP_POS_CHANGED
} from "../lvgl-constants";
//import { json } from "stream/consumers";

////////////////////////////////////////////////////////////////////////////////

export const LVGL_SPINBOX_STEP_DIRECTION: { [key: string]: number } = {
    left: LV_DIR_LEFT,
    right: LV_DIR_RIGHT
};

export class LVGLSpinboxWidget extends LVGLWidget {
    static classInfo = makeDerivedClassInfo(LVGLWidget.classInfo, {
        enabledInComponentPalette: (projectType: ProjectType) =>
            projectType === ProjectType.LVGL,

        componentPaletteGroupName: "!1Input",

        properties: [
            ...makeLvglExpressionProperty(
                "digitCount",
                "integer",
                "input",
                ["literal", "expression"],
                {
                    propertyGridGroup: specificGroup
                }
            ),
            ...makeLvglExpressionProperty(
                "separatorPosition",
                "integer",
                "input",
                ["literal", "expression"],
                {
                    propertyGridGroup: specificGroup
                }
            ),
            ...makeLvglExpressionProperty(
                "min",
                "integer",
                "input",
                ["literal", "expression"],
                {
                    propertyGridGroup: specificGroup
                }
            ),
            ...makeLvglExpressionProperty(
                "max",
                "integer",
                "input",
                ["literal", "expression"],
                {
                    propertyGridGroup: specificGroup
                }
            ),
            {
                name: "rollover",
                type: PropertyType.Boolean,
                propertyGridGroup: specificGroup,
                checkboxStyleSwitch: true
            },
            {
                name: "stepDirection",
                type: PropertyType.Enum,
                enumItems: [
                    {
                        id: "right",
                        label: "RIGHT"
                    },
                    {
                        id: "left",
                        label: "LEFT"
                    }
                ],
                enumDisallowUndefined: true,
                propertyGridGroup: specificGroup
            },
            ...makeLvglExpressionProperty(
                "step",
                "integer",
                "assignable",
                ["literal", "expression"],
                {
                    propertyGridGroup: specificGroup
                }
            ),
            ...makeLvglExpressionProperty(
                "value",
                "integer",
                "assignable",
                ["literal", "expression"],
                {
                    propertyGridGroup: specificGroup
                }
            )
        ],

        defaultValue: {
            left: 0,
            top: 0,
            width: 180,
            height: 100,
            clickableFlag: true,
            digitCount: 5,
            digitCountType: "literal",
            separatorPosition: 0,
            separatorPositionType: "literal",
            min: -99999,
            minType: "literal",
            max: 99999,
            maxType: "literal",
            rollover: false,
            stepDirection: "right",
            step: 0,
            stepType: "literal",
            value: 0,
            valueType: "literal"
        },

        beforeLoadHook(
            object: LVGLSpinboxWidget,
            jsObject: Partial<LVGLSpinboxWidget>
        ) {
            if (jsObject.digitCount == undefined) {
                jsObject.digitCount = 5;
            }
            if (jsObject.digitCountType == undefined) {
                jsObject.digitCountType = "literal";
            }
            if (jsObject.separatorPosition == undefined) {
                jsObject.separatorPosition = 0;
            }
            if (jsObject.separatorPositionType == undefined) {
                jsObject.separatorPositionType = "literal";
            }
            if (jsObject.min == undefined) {
                jsObject.min = -99999;
            }
            if (jsObject.minType == undefined) {
                jsObject.minType = "literal";
            }
            if (jsObject.max == undefined) {
                jsObject.max = 99999;
            }
            if (jsObject.maxType == undefined) {
                jsObject.maxType = "literal";
            }
            if (jsObject.rollover == undefined) {
                jsObject.rollover = false;
            }
            if (jsObject.stepDirection == undefined) {
                jsObject.stepDirection = "right";
            }
            if (jsObject.step == undefined) {
                jsObject.step = 1;
            }
            if (jsObject.stepType == undefined) {
                jsObject.stepType = "literal";
            }
            if (jsObject.value == undefined) {
                jsObject.value = 0;
            }
            if (jsObject.valueType == undefined) {
                jsObject.valueType = "literal";
            }
        },

        icon: (
            <svg viewBox="0 0 24 24"
                stroke="currentColor"
                fill="currentColor"
            >
                <path
                    fillRule="evenodd"
                    clipRule="evenodd"
                    d="M11.47 4.72a.75.75 0 0 1 1.06 0l3.75 3.75a.75.75 0 0 1-1.06 1.06L12 6.31 8.78 9.53a.75.75 0 0 1-1.06-1.06zm-3.75 9.75a.75.75 0 0 1 1.06 0L12 17.69l3.22-3.22a.75.75 0 1 1 1.06 1.06l-3.75 3.75a.75.75 0 0 1-1.06 0l-3.75-3.75a.75.75 0 0 1 0-1.06"
                    fill="#0F172A"
                />
            </svg>
        ),

        lvgl: (widget: LVGLSpinboxWidget, project: Project) => {
            return {
                parts: ["MAIN", "SELECTED", "CURSOR"],
                defaultFlags:
                    project.settings.general.lvglVersion == "9.0"
                        ? "CLICKABLE|CLICK_FOCUSABLE|GESTURE_BUBBLE|PRESS_LOCK|SCROLLABLE|SCROLL_CHAIN_HOR|SCROLL_CHAIN_VER|SCROLL_ELASTIC|SCROLL_MOMENTUM|SCROLL_ON_FOCUS|SNAPPABLE"
                        : "CLICKABLE|CLICK_FOCUSABLE|GESTURE_BUBBLE|PRESS_LOCK|SCROLLABLE|SCROLL_CHAIN_HOR|SCROLL_CHAIN_VER|SCROLL_ELASTIC|SCROLL_MOMENTUM|SCROLL_ON_FOCUS|SCROLL_WITH_ARROW|SNAPPABLE",
                states: ["CHECKED", "DISABLED", "FOCUSED", "PRESSED"]
            };
        }
    });

    digitCount: number;
    digitCountType: LVGLPropertyType;
    separatorPosition: number;
    separatorPositionType: LVGLPropertyType;
    min: number;
    minType: LVGLPropertyType;
    max: number;
    maxType: LVGLPropertyType;
    rollover: boolean;
    stepDirection: string;
    step: number | string;
    stepType: LVGLPropertyType;
    value: number | string;
    valueType: LVGLPropertyType;

    override makeEditable() {
        super.makeEditable();

        makeObservable(this, {
            digitCount: observable,
            digitCountType: observable,
            separatorPosition: observable,
            separatorPositionType: observable,
            min: observable,
            minType: observable,
            max: observable,
            maxType: observable,
            rollover: observable,
            stepDirection: observable,
            step: observable,
            stepType: observable,
            value: observable,
            valueType: observable
        });
    }

    override getIsAccessibleFromSourceCode() {
        return (
            this.digitCountType == "expression" ||
            this.separatorPositionType == "expression" ||
            this.minType == "expression" ||
            this.maxType == "expression" ||
            this.valueType == "expression" ||
            this.stepType == "expression"
        );
    }

    override get hasEventHandler() {
        return (
            super.hasEventHandler ||
            this.digitCountType == "expression" ||
            this.separatorPositionType == "expression" ||
            this.minType == "expression" ||
            this.maxType == "expression" ||
            this.valueType == "expression" ||
            this.stepType == "expression"
        );
    }

    override lvglCreateObj(
        runtime: LVGLPageRuntime,
        parentObj: number
    ): number {
        const digitCountExpr = getExpressionPropertyData(runtime, this, "digitCount");
        const separatorPositionExpr = getExpressionPropertyData(runtime, this, "separatorPosition");
        const minExpr = getExpressionPropertyData(runtime, this, "min");
        const maxExpr = getExpressionPropertyData(runtime, this, "max");
        const stepExpr = getExpressionPropertyData(runtime, this, "step");
        const valueExpr = getExpressionPropertyData(runtime, this, "value");

        const rect = this.getLvglCreateRect();

        const obj = runtime.wasm._lvglCreateSpinbox(
            parentObj,
            runtime.getWidgetIndex(this),

            rect.left,
            rect.top,
            rect.width,
            rect.height,

            digitCountExpr
                ? 5
                : this.digitCountType == "expression"
                    ? 5
                    : (this.digitCount as number),

            separatorPositionExpr
                ? 1
                : this.separatorPositionType == "expression"
                    ? 1
                    : (this.separatorPosition as number),

            minExpr
                ? 0
                : this.minType == "expression"
                    ? 0
                    : (this.min as number),

            maxExpr
                ? 9999
                : this.maxType == "expression"
                    ? 9999
                    : (this.max as number),

            this.rollover,
            LVGL_SPINBOX_STEP_DIRECTION[this.stepDirection],
            stepExpr
                ? 1
                : this.stepType == "expression"
                    ? 1
                    : (this.step as number),

            valueExpr
                ? 0
                : this.valueType == "expression"
                    ? 0
                    : (this.value as number)
        );

        if (digitCountExpr) {
            runtime.wasm._lvglUpdateSpinboxDigitCounter(
                obj,
                getFlowStateAddressIndex(runtime),
                digitCountExpr.componentIndex,
                digitCountExpr.propertyIndex
            );
        }

        if (separatorPositionExpr) {
            runtime.wasm._lvglUpdateSpinboxSeparatorPosition(
                obj,
                getFlowStateAddressIndex(runtime),
                separatorPositionExpr.componentIndex,
                separatorPositionExpr.propertyIndex
            );
        }

        if (minExpr) {
            runtime.wasm._lvglUpdateSpinboxMin(
                obj,
                getFlowStateAddressIndex(runtime),
                minExpr.componentIndex,
                minExpr.propertyIndex
            );
        }

        if (maxExpr) {
            runtime.wasm._lvglUpdateSpinboxMax(
                obj,
                getFlowStateAddressIndex(runtime),
                maxExpr.componentIndex,
                maxExpr.propertyIndex
            );
        }

        if (stepExpr) {
            runtime.wasm._lvglUpdateSpinboxStep(
                obj,
                getFlowStateAddressIndex(runtime),
                stepExpr.componentIndex,
                stepExpr.propertyIndex
            );
        }

        if (valueExpr) {
            runtime.wasm._lvglUpdateSpinboxValue(
                obj,
                getFlowStateAddressIndex(runtime),
                valueExpr.componentIndex,
                valueExpr.propertyIndex
            );
        }

        return obj;
    }

    override createEventHandlerSpecific(runtime: LVGLPageRuntime, obj: number) {
        const digitCounterExpr = getExpressionPropertyData(runtime, this, "digitCounter");
        if (digitCounterExpr) {
            lvglAddObjectFlowCallback(
                runtime,
                obj,
                LV_EVENT_SPINBOX_DIGIT_COUNTER_CHANGED,
                digitCounterExpr.componentIndex,
                digitCounterExpr.propertyIndex,
                0
            );
        }
        const separatorPositionExpr = getExpressionPropertyData(runtime, this, "separatorPosition");
        if (separatorPositionExpr) {
            lvglAddObjectFlowCallback(
                runtime,
                obj,
                LV_EVENT_SPINBOX_SEP_POS_CHANGED,
                separatorPositionExpr.componentIndex,
                separatorPositionExpr.propertyIndex,
                0
            );
        }
        const minExpr = getExpressionPropertyData(runtime, this, "min");
        if (minExpr) {
            lvglAddObjectFlowCallback(
                runtime,
                obj,
                LV_EVENT_SPINBOX_MIN_CHANGED,
                minExpr.componentIndex,
                minExpr.propertyIndex,
                0
            );
        }
        const maxExpr = getExpressionPropertyData(runtime, this, "max");
        if (maxExpr) {
            lvglAddObjectFlowCallback(
                runtime,
                obj,
                LV_EVENT_SPINBOX_MAX_CHANGED,
                maxExpr.componentIndex,
                maxExpr.propertyIndex,
                0
            );
        }
        const stepExpr = getExpressionPropertyData(runtime, this, "step");
        if (stepExpr) {
            lvglAddObjectFlowCallback(
                runtime,
                obj,
                LV_EVENT_SPINBOX_STEP_CHANGED,
                stepExpr.componentIndex,
                stepExpr.propertyIndex,
                0
            );
        }

        const valueExpr = getExpressionPropertyData(runtime, this, "value");
        if (valueExpr) {
            lvglAddObjectFlowCallback(
                runtime,
                obj,
                LV_EVENT_SPINBOX_VALUE_CHANGED,
                valueExpr.componentIndex,
                valueExpr.propertyIndex,
                0
            );
        }
    }

    override lvglBuildObj(build: LVGLBuild) {
        build.line(`lv_obj_t *obj = lv_spinbox_create(parent_obj);`);
    }

    override lvglBuildSpecific(build: LVGLBuild) {
        if (this.digitCountType == "literal" && this.separatorPositionType == "literal") {
            build.line(
                `lv_spinbox_set_digit_format(obj, ${this.digitCount}, ${this.separatorPosition});`
            );
        }

        if (this.minType == "literal" && this.maxType == "literal") {
            build.line(`lv_spinbox_set_range(obj, ${this.min}, ${this.max});`);
        }

        build.line(
            `lv_spinbox_set_rollover(obj, ${this.rollover ? "true" : "false"});`
        );

        build.line(
            `lv_spinbox_set_digit_step_direction(obj, ${this.stepDirection == "left" ? "LV_DIR_LEFT" : "LV_DIR_RIGHT"});`
        );

        if (this.stepType == "literal") {
            if (this.step != 0) {
                build.line(`lv_spinbox_set_step(obj, ${this.step});`);
            }
        }

        if (this.valueType == "literal") {
            if (this.value != 0) {
                build.line(`lv_spinbox_set_value(obj, ${this.value});`);
            }
        }
    }

    override lvglBuildTickSpecific(build: LVGLBuild) {
        expressionPropertyBuildTickSpecific<LVGLSpinboxWidget>(
            build,
            this,
            "digitCount" as const,
            "",
            "lv_spinbox_set_digit_format",
            undefined,
            "count"
        );

        expressionPropertyBuildTickSpecific<LVGLSpinboxWidget>(
            build,
            this,
            "separatorPosition" as const,
            "",
            "lv_spinbox_set_digit_format",
            undefined,
            "position"
        );

        expressionPropertyBuildTickSpecific<LVGLSpinboxWidget>(
            build,
            this,
            "min" as const,
            "",
            "lv_spinbox_set_range",
            undefined,
            "min"
        );

        expressionPropertyBuildTickSpecific<LVGLSpinboxWidget>(
            build,
            this,
            "max" as const,
            "",
            "lv_spinbox_set_range",
            undefined,
            "max"
        );

        expressionPropertyBuildTickSpecific<LVGLSpinboxWidget>(
            build,
            this,
            "step" as const,
            "lv_spinbox_get_step",
            "lv_spinbox_set_step"
        );

        expressionPropertyBuildTickSpecific<LVGLSpinboxWidget>(
            build,
            this,
            "value" as const,
            "lv_spinbox_get_value",
            "lv_spinbox_set_value"
        );
    }

    override buildEventHandlerSpecific(build: LVGLBuild) {
        expressionPropertyBuildEventHandlerSpecific<LVGLSpinboxWidget>(
            build,
            this,
            "step" as const,
            "lv_spinbox_get_step"
        );

        expressionPropertyBuildEventHandlerSpecific<LVGLSpinboxWidget>(
            build,
            this,
            "value" as const,
            "lv_spinbox_get_value"
        );
    }
}
