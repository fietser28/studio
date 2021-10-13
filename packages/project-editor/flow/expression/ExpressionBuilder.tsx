import React from "react";
import ReactDOM from "react-dom";
import { computed, observable, action } from "mobx";
import { observer } from "mobx-react";

import { _map } from "eez-studio-shared/algorithm";
import { humanize } from "eez-studio-shared/string";

import { ITreeNode, Tree } from "eez-studio-ui/tree";

import {
    getAncestorOfType,
    getProperty,
    IEezObject,
    IOnSelectParams,
    PropertyInfo
} from "project-editor/core/object";
import { ProjectContext } from "project-editor/project/context";
import { getDocumentStore } from "project-editor/core/store";
import { Dialog, showDialog } from "eez-studio-ui/dialog";
import { Component } from "project-editor/flow/component";
import { getFlow } from "project-editor/project/project";
import {
    binaryOperators,
    builtInConstants,
    builtInFunctions,
    logicalOperators,
    unaryOperators
} from "./operations";
import {
    getArrayElementTypeFromType,
    getStructureFromType,
    isArrayType,
    isStructType,
    humanizeVariableType
} from "project-editor/features/variable/value-type";

export async function expressionBuilder(
    object: IEezObject,
    propertyInfo: PropertyInfo,
    opts: {
        assignableExpression: boolean;
        title: string;
        width: number;
        height?: number;
    },
    params?: IOnSelectParams
) {
    let disposed = false;

    return new Promise<{
        [propertyName: string]: string;
    }>((resolve, reject) => {
        const onDispose = () => {
            if (!disposed) {
                ReactDOM.unmountComponentAtNode(element);
                if (modalDialog) {
                    modalDialog.close();
                }
                disposed = true;
            }
        };

        const onOk = (value: any) => {
            resolve(value);
            onDispose();
        };

        const [modalDialog, element] = showDialog(
            <ProjectContext.Provider value={getDocumentStore(object)}>
                <SelectItemDialog
                    object={object}
                    propertyInfo={propertyInfo}
                    assignableExpression={opts.assignableExpression}
                    params={params}
                    onOk={onOk}
                    onCancel={onDispose}
                />
            </ProjectContext.Provider>,
            {
                jsPanel: Object.assign({}, opts)
            }
        );
    });
}

const VariableLabel = observer(
    ({ name, type }: { name: string; type: string }) => (
        <>
            <span className="name">{name}</span>
            <span className="type">{type}</span>
        </>
    )
);

@observer
class SelectItemDialog extends React.Component<{
    object: IEezObject;
    propertyInfo: PropertyInfo;
    assignableExpression: boolean;
    params?: IOnSelectParams;
    onOk: (value: any) => void;
    onCancel: () => void;
}> {
    static contextType = ProjectContext;
    declare context: React.ContextType<typeof ProjectContext>;

    @observable selection: string | undefined;

    onOkEnabled = () => {
        return this.selection != undefined;
    };

    onOk = () => {
        const { object, propertyInfo, params } = this.props;

        if (!this.selection) {
            return;
        }

        let value = this.selection;

        if (
            params &&
            params.textInputSelection &&
            params.textInputSelection.start != null &&
            params.textInputSelection.end != null
        ) {
            const existingValue: string =
                getProperty(object, propertyInfo.name) || "";
            value =
                existingValue.substring(0, params.textInputSelection.start) +
                value +
                existingValue.substring(params.textInputSelection.end);
        }

        this.props.onOk({
            [propertyInfo.name]: value
        });

        return true;
    };

    @computed get component() {
        return getAncestorOfType(
            this.props.object,
            Component.classInfo
        ) as Component;
    }

    @computed get flow() {
        return getFlow(this.component);
    }

    @computed get componentInputs() {
        return this.component.inputs.filter(
            componentInput => !componentInput.name.startsWith("@")
        );
    }

    @computed get localVariables() {
        return this.flow.localVariables;
    }

    @computed get globalVariables() {
        return this.context.project.variables.globalVariables;
    }

    getTypeChildren(type: string, prefix: string): ITreeNode<string>[] {
        if (isArrayType(type)) {
            return this.getTypeChildren(
                getArrayElementTypeFromType(type)!,
                `${prefix}[]`
            );
        } else if (isStructType(type)) {
            const structure = getStructureFromType(this.context.project, type);
            if (structure) {
                return structure.fields.map(field => {
                    const data = `${prefix}.${field.name}`;
                    return {
                        id: field.name,
                        label: (
                            <VariableLabel
                                name={
                                    (prefix.endsWith("[]") ? "[]" : "") +
                                    "." +
                                    field.name
                                }
                                type={humanizeVariableType(field.type)}
                            />
                        ),
                        children: this.getTypeChildren(
                            field.type,
                            `${prefix}.${field.name}`
                        ),
                        selected: this.selection == data,
                        expanded: true,
                        data: data
                    };
                });
            }
        }

        return [];
    }

    getOperators<
        T extends {
            [operator: string]: {
                name: string;
            };
        }
    >(operators: T) {
        return _map(operators, (operator, operatorSign) => ({
            id: operator.name,
            label: `${humanize(operator.name)} (${operatorSign})`,
            children: [],
            selected: this.selection == operatorSign,
            expanded: false,
            data: operator
        }));
    }

    @computed get rootNode(): ITreeNode<string> {
        const children: ITreeNode<string>[] = [];

        if (!this.props.assignableExpression && this.componentInputs.length) {
            children.push({
                id: "component-inputs",
                label: "Component inputs",
                children: this.componentInputs.map(componentInput => ({
                    id: componentInput.name,
                    label: (
                        <VariableLabel
                            name={componentInput.name}
                            type={componentInput.type}
                        />
                    ),
                    children: [],
                    selected: this.selection == componentInput.name,
                    expanded: false,
                    data: componentInput.name
                })),
                selected: false,
                expanded: true
            });
        }

        if (this.localVariables.length) {
            children.push({
                id: "local-variables",
                label: "Local variables",
                children: this.localVariables.map(localVariable => ({
                    id: localVariable.name,
                    label: (
                        <VariableLabel
                            name={localVariable.name}
                            type={humanizeVariableType(localVariable.type)}
                        />
                    ),
                    children: this.getTypeChildren(
                        localVariable.type,
                        localVariable.name
                    ),
                    selected: this.selection == localVariable.name,
                    expanded: true,
                    data: localVariable.name
                })),
                selected: false,
                expanded: true
            });
        }

        if (this.globalVariables.length) {
            children.push({
                id: "global-variables",
                label: "Global variables",
                children: this.globalVariables.map(globalVariable => ({
                    id: globalVariable.name,
                    label: (
                        <VariableLabel
                            name={globalVariable.name}
                            type={humanizeVariableType(globalVariable.type)}
                        />
                    ),
                    children: this.getTypeChildren(
                        globalVariable.type,
                        globalVariable.name
                    ),
                    selected: this.selection == globalVariable.name,
                    expanded: true,
                    data: globalVariable.name
                })),
                selected: false,
                expanded: true
            });
        }

        if (!this.props.assignableExpression) {
            if (this.context.project.variables.enums.length) {
                children.push({
                    id: "enumerations",
                    label: "Enumerations",
                    children: this.context.project.variables.enums.map(
                        enumeration => ({
                            id: enumeration.name,
                            label: enumeration.name,
                            children: enumeration.members.map(member => {
                                const data = `${enumeration.name}.${member.name}`;
                                return {
                                    id: member.name,
                                    label: member.name,
                                    children: [],
                                    selected: this.selection == member.name,
                                    expanded: false,
                                    data
                                };
                            }),
                            selected: false,
                            expanded: true,
                            data: undefined
                        })
                    ),
                    selected: false,
                    expanded: true
                });
            }

            children.push({
                id: "binary-operators",
                label: "Binary operators",
                children: this.getOperators(binaryOperators),
                selected: false,
                expanded: true
            });

            children.push({
                id: "logical-operators",
                label: "Logical operators",
                children: this.getOperators(logicalOperators),
                selected: false,
                expanded: true
            });

            children.push({
                id: "unary-operators",
                label: "Unary operators",
                children: this.getOperators(unaryOperators),
                selected: false,
                expanded: true
            });

            children.push({
                id: "built-in-functions",
                label: "Built-in Functions",
                children: _map(builtInFunctions, (func, functionName) => {
                    const data = `${functionName}(${func.args
                        .map(arg => `<${arg}>`)
                        .join(",")})`;
                    return {
                        id: functionName,
                        label: functionName,
                        children: [],
                        selected: this.selection == data,
                        expanded: false,
                        data
                    };
                }),
                selected: false,
                expanded: true
            });

            children.push({
                id: "built-in-constants",
                label: "Built-in Constants",
                children: _map(builtInConstants, (constant, constantName) => ({
                    id: constantName,
                    label: constantName,
                    children: [],
                    selected: this.selection == constantName,
                    expanded: false,
                    data: constantName
                })),
                selected: false,
                expanded: true
            });
        }

        return observable({
            id: "all",
            label: "All",
            children,
            selected: false,
            expanded: true
        });
    }

    selectNode = action((node?: ITreeNode<string>) => {
        this.selection = node && node.data;
    });

    render() {
        return (
            <Dialog
                modal={false}
                okButtonText="Select"
                okEnabled={this.onOkEnabled}
                onOk={this.onOk}
                onCancel={this.props.onCancel}
            >
                <div className="EezStudio_ExpressionBuilder">
                    <Tree
                        showOnlyChildren={true}
                        rootNode={this.rootNode}
                        selectNode={this.selectNode}
                        onDoubleClick={this.onOk}
                    />
                </div>
            </Dialog>
        );
    }
}