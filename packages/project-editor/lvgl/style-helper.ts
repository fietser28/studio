import tinycolor from "tinycolor2";

import { IEezObject, PropertyType } from "project-editor/core/object";

import {
    BUILT_IN_FONTS,
    lvglPropertiesMap,
    LVGLPropertyInfo,
    text_font_property_info
} from "project-editor/lvgl/style-catalog";
import type { LVGLPageRuntime } from "project-editor/lvgl/page-runtime";
import { LVGLParts, lvglStates } from "project-editor/lvgl/lvgl-constants";
import { getLvglParts } from "project-editor/lvgl/lvgl-versions";

////////////////////////////////////////////////////////////////////////////////

export function getPartCode(object: IEezObject, part: string) {
    const lvglParts = getLvglParts(object);

    if (part.startsWith("custom")) {
        const partIndex = Number.parseInt(part.substring("custom".length));
        const custom1Code = lvglParts.CUSTOM1;
        return custom1Code + (partIndex - 1);
    } else {
        const partCode = (lvglParts as any)[part];
        if (partCode == undefined) {
            console.error("UNEXPECTED!");
            return 0;
        }
        return partCode;
    }
}

export function getPartBuildCode(part: string) {
    return "LV_PART_" + part;
}

export function getStateCode(state: string) {
    const stateCode = (lvglStates as any)[state];
    if (stateCode == undefined) {
        console.error("UNEXPECTED!");
        return 0;
    }
    return stateCode;
}

export function getStateBuildCode(state: string) {
    return "LV_STATE_" + state;
}

export function getSelectorCode(
    object: IEezObject,
    partStr: string,
    statesStr: string
) {
    const partCode = getPartCode(object, partStr);

    const statesCode =
        statesStr.trim() != ""
            ? statesStr
                  .split("|")
                  .reduce((previousCode: number, currentStateStr: string) => {
                      return previousCode | getStateCode(currentStateStr);
                  }, 0)
            : 0;

    return partCode | statesCode;
}

export function getSelectorBuildCode(partStr: string, statesStr: string) {
    const partCode = getPartBuildCode(partStr);

    const statesCode =
        statesStr.trim() != ""
            ? statesStr
                  .split("|")
                  .map(state => getStateBuildCode(state))
                  .join(" | ")
            : "0";

    return `${partCode} | ${statesCode}`;
}

export function getPropertyInfo(propertyName: string) {
    return lvglPropertiesMap.get(propertyName);
}

export function colorNumToRgb(color: number): string {
    // signed to unsigned
    color = color >>> 0;

    // color is in BGR format
    const b = (color >> 0) & 0xff;
    const g = (color >> 8) & 0xff;
    const r = (color >> 16) & 0xff;

    let result = (r << 16) | (g << 8) | (b << 0);

    // signed to unsigned
    result = result >>> 0;

    return "#" + result.toString(16).padStart(6, "0");
}

export function colorRgbToNum(color: string): number {
    const rgb = tinycolor(color).toRgb();

    // result is in BGR format
    let result = (rgb.b << 0) | (rgb.g << 8) | (rgb.r << 16) | (255 << 24);

    // signed to unsigned
    result = result >>> 0;

    return result;
}

export function colorRgbToHexNumStr(color: string) {
    return "0x" + colorRgbToNum(color).toString(16).padStart(8, "0");
}

export function getStylePropDefaultValue(
    runtime: LVGLPageRuntime | undefined,
    lvglObj: number | undefined,
    part: LVGLParts,
    state: string,
    propertyInfo: LVGLPropertyInfo
) {
    if (runtime && lvglObj) {
        if (propertyInfo.type == PropertyType.ThemedColor) {
            let colorNum = runtime.wasm._lvglObjGetStylePropColor(
                lvglObj,
                getPartCode(runtime.page, part),
                getStateCode(state),
                runtime.getLvglStylePropCode(propertyInfo.lvglStyleProp.code)
            );
            return colorNumToRgb(colorNum);
        } else if (
            propertyInfo.type == PropertyType.Number ||
            propertyInfo.type == PropertyType.Enum
        ) {
            if (propertyInfo == text_font_property_info) {
                let fontIndex = runtime.wasm._lvglObjGetStylePropBuiltInFont(
                    lvglObj,
                    getPartCode(runtime.page, part),
                    getStateCode(state),
                    runtime.getLvglStylePropCode(
                        propertyInfo.lvglStyleProp.code
                    )
                );

                if (fontIndex != -1) {
                    return BUILT_IN_FONTS[fontIndex];
                } else {
                    let fontAddr = runtime.wasm._lvglObjGetStylePropFontAddr(
                        lvglObj,
                        getPartCode(runtime.page, part),
                        getStateCode(state),
                        runtime.getLvglStylePropCode(
                            propertyInfo.lvglStyleProp.code
                        )
                    );
                    const font = runtime.fontAddressToFont.get(fontAddr);
                    if (font) {
                        return font.name;
                    }
                }
            } else {
                let num = runtime.wasm._lvglObjGetStylePropNum(
                    lvglObj,
                    getPartCode(runtime.page, part),
                    getStateCode(state),
                    runtime.getLvglStylePropCode(
                        propertyInfo.lvglStyleProp.code
                    )
                );
                return propertyInfo.lvglStyleProp.valueRead
                    ? propertyInfo.lvglStyleProp.valueRead(num)
                    : num;
            }
        } else if (propertyInfo.type == PropertyType.Boolean) {
            let num = runtime.wasm._lvglObjGetStylePropNum(
                lvglObj,
                getPartCode(runtime.page, part),
                getStateCode(state),
                runtime.getLvglStylePropCode(propertyInfo.lvglStyleProp.code)
            );
            return num ? true : false;
        }
    }

    return 0;
}
