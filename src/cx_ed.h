#ifndef CX_ED_H
#define CX_ED_H

#include "cx_ed_action.h"

// action history
// entity names
// selected entity
// draw selected entity bounds
// draw gizmos for selected entity
// gizmo dragging, snapping
//
//
// actions:
//  - edit entity transform
//  - create entity
//  - delete entity
//  - add entity component
//  - delete entity component

void cx_ed_init(void);
void cx_ed_update(double dt_seconds);
void cx_ed_draw(float aspect);

#endif
