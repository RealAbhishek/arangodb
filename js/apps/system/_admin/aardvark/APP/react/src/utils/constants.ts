import { Dispatch, ReactNode } from "react";

export type ChildProp = {
  children?: ReactNode;
};

export type State<FormState> = {
  formState: FormState;
  formCache: object;
  show: boolean;
  showJsonForm: boolean;
  lockJsonForm: boolean;
  renderKey: string;
};

export type DispatchArgs<FormState> = {
  type: string;
  field?: {
    path: string;
    value?: any;
  };
  basePath?: string;
  formState?: FormState;
};

export type RenderView = "LinkList" | "AddNew" | "ViewParent" | "ViewChild" | "ViewField";

export type FormProps<FormState> = {
  formState: FormState;
  dispatch: Dispatch<DispatchArgs<FormState>>;
  disabled?: boolean;
  view?: string;
  show?: RenderView;
};

