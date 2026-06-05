/**
 * BK 16:9 HUD Stretch Mod
 */

#include "modding.h"
#include "functions.h"
#include "variables.h"
#include "rt64_extended_gbi.h"
#include "core2/gc/zoombox.h"
#include "core2/modelRender.h"
#include "core1/viewport.h"

#ifndef G_EX_MATRIX_FLOAT_V1
#define G_EX_MATRIX_FLOAT_V1 0x000030
#endif

#ifndef gEXMatrixFloat
#define gEXMatrixFloat(cmd, m, p) \
    G_EX_COMMAND2(cmd, \
        PARAM(RT64_EXTENDED_OPCODE, 8, 24) | PARAM(G_EX_MATRIX_FLOAT_V1, 24, 0), \
        PARAM((p), 8, 0), \
        0, \
        (unsigned)(m) \
    )
#endif

#ifndef gEXMatrixGroupSimpleNormal
#define gEXMatrixGroupSimpleNormal(cmd, id, push, proj, edit) \
    gEXMatrixGroup(cmd, id, G_EX_INTERPOLATE_SIMPLE, push, proj, \
        G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, \
        G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, \
        G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_ORDER_LINEAR, \
        edit, G_EX_ASPECT_AUTO, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP)
#endif

#ifndef gEXMatrixGroupSimpleVerts
#define gEXMatrixGroupSimpleVerts(cmd, id, push, proj, edit) \
    gEXMatrixGroup(cmd, id, G_EX_INTERPOLATE_SIMPLE, push, proj, \
        G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_INTERPOLATE, \
        G_EX_COMPONENT_INTERPOLATE, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, \
        G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP, G_EX_ORDER_LINEAR, \
        edit, G_EX_ASPECT_AUTO, G_EX_COMPONENT_SKIP, G_EX_COMPONENT_SKIP)
#endif

#define HUD_AIRSCORE_TRANSFORM_ID_START 100

extern f32 sViewportAspect;
extern Vp sViewportStack[];
extern s32 sViewportStackIndex;
extern u16 gScissorBoxRight;
extern u16 gScissorBoxTop;
extern u16 gScissorBoxBottom;

extern void viewport_setRenderViewportAndPerspectiveMatrix(Gfx **gfx, Mtx **mtx);
extern void func_8033A308(f32 *arg0);
extern void anctrl_drawSetup(AnimCtrl *ctrl, f32 *pos, s32 arg2);
extern void gcpausemenu_zoombox_callback(s32 portrait_id, s32 zoombox_state);

static float identity_matrix[4][4] = {
    { 1.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f }
};

RECOMP_FORCE_PATCH void viewport_setRenderViewportAndOrthoMatrix(Gfx **gfx, Mtx **mtx) {
    f32 stretch_x = (16.0f / 9.0f) / (4.0f / 3.0f);
    
    gEXSetScissor((*gfx)++, G_SC_NON_INTERLACE, G_EX_ORIGIN_LEFT, G_EX_ORIGIN_RIGHT, 
                  0, gScissorBoxRight, 0, gScissorBoxBottom);
    
    gSPViewport((*gfx)++, &sViewportStack[sViewportStackIndex]);

    guOrthoF(*mtx, 
             -(2 * (f32)gFramebufferWidth) / stretch_x, 
             (2 * (f32)gFramebufferWidth) / stretch_x, 
             -(2 * (f32)gFramebufferHeight), 
             (2 * (f32)gFramebufferHeight), 
             1.0f, 20.0f, 1.0f);
    
    gEXMatrixFloat((*gfx)++, OS_K0_TO_PHYSICAL((*mtx)++), 
                   G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);

    guTranslate(*mtx, 0.0f, 0.0f, 0.0f);
    gSPMatrix((*gfx)++, OS_K0_TO_PHYSICAL((*mtx)++), 
              G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

    gEXSetViewMatrixFloat((*gfx)++, identity_matrix);
    gEXMatrixGroupSimpleNormal((*gfx)++, G_EX_ID_AUTO, G_EX_NOPUSH, 
                               G_MTX_PROJECTION, G_EX_EDIT_NONE);
}

RECOMP_FORCE_PATCH void func_803163A8(GcZoombox *this, Gfx **gfx, Mtx **mtx) {
    f32 sp5C[3], sp50[3], sp44[3], sp38[3], sp34;

    f32 saved_aspect = sViewportAspect;
    f32 stretch_factor = (16.0f / 9.0f) / (4.0f / 3.0f);
    sViewportAspect /= stretch_factor;
    
    viewport_setRenderViewportAndPerspectiveMatrix(gfx, mtx);
    sp34 = viewport_transformCoordinate((f32)this->unk170, (f32)this->unk172, sp50, sp5C);
    
    sViewportAspect = saved_aspect;

    if (this->unk1A4_24) {
        sp5C[1] += 180.0f;
        sp5C[0] -= 2 * sp5C[0];
    }
    
    sp38[0] = 0.0f; sp38[1] = 0.0f; sp38[2] = 0.0f;
    sp44[0] = 0.0f; sp44[1] = 0.0f; sp44[2] = 0.0f;
    
    func_8033A308(sp44);
    modelRender_setDepthMode(MODEL_RENDER_DEPTH_NONE);
    
    if (this->anim_ctrl != NULL) {
        anctrl_drawSetup(this->anim_ctrl, sp50, 1);
    }

    modelRender_draw(gfx, mtx, sp50, sp5C, this->unk198 * sp34, sp38, this->model);
}

// Override BanjoRecomp's air counter function with matrix groups for bug fix
RECOMP_FORCE_PATCH void fxairscore_draw(enum item_e item_id, struct8s *arg1, Gfx **gfx, Mtx **mtx, Vtx **vtx) {
    extern BKSprite *s_sprite;
    extern Gfx s_fxairscore_context[];
    extern f32 s_texture_scale;
    extern f32 s_active_count;
    extern f32 D_80381F68[];
    extern void func_80347FC0(Gfx **gfx, BKSprite *sprite, s32 frame, s32 tmem, s32 rtile, s32 uls, s32 ult, s32 cms, s32 cmt, s32 *width, s32 *height);
    extern f32 func_802FB0E4(struct8s *this);

    #define AIRSCORE_COUNT (6)

    f32 y;
    f32 x;
    s32 texture_width;
    s32 texture_height;
    s32 i_part;
    s32 var_s6;
    s32 v_x;
    s32 v_y;

    if (s_sprite != 0) {
        gSPDisplayList((*gfx)++, s_fxairscore_context);
        func_80347FC0(gfx, s_sprite, 0, 0, 0, 0, 0, 2, 2, &texture_width, &texture_height);

        viewport_setRenderViewportAndOrthoMatrix(gfx, mtx);
        for (i_part = 0; i_part < AIRSCORE_COUNT; i_part++) {
            if ((i_part != 0) && (i_part != 5)) {
                var_s6 = (i_part & 1) ? i_part + 1 : i_part - 1;
            }
            else {
                var_s6 = i_part;
            }
            gDPPipeSync((*gfx)++);
            if ((f32)(5 - i_part) < s_active_count) {
                gDPSetPrimColor((*gfx)++, 0, 0, 0x00, 0x00, 0x00, 0xFF);
            }
            else {
                gDPSetPrimColor((*gfx)++, 0, 0, 0x00, 0x00, 0x00, 0x78);
            }
            x = func_802FB0E4(arg1);
            x = ((-40 + x) + D_80381F68[var_s6]) - ((f32)gFramebufferWidth / 2);
            y = ((78 + (i_part * 15.5)) - ((f32)gFramebufferHeight / 2));

            // Assign a matrix group to each piece of air for bug fix
            gEXMatrixGroupSimpleVerts((*gfx)++, HUD_AIRSCORE_TRANSFORM_ID_START + i_part, G_EX_PUSH, G_MTX_MODELVIEW, G_EX_EDIT_NONE);

            x = (i_part & 1) ? x + 5.0f : x - 5.0f;
            gSPVertex((*gfx)++, *vtx, 4, 0);
            for (v_y = 0; v_y < 2; v_y++) {
                for (v_x = 0; v_x < 2; v_x++) {
                    (*vtx)->v.ob[0] = (x + (((texture_width * s_texture_scale) * v_x) - ((texture_width * s_texture_scale) / 2))) * 4.0f;
                    (*vtx)->v.ob[1] = (y + (((texture_height * s_texture_scale) / 2) - (texture_height * s_texture_scale) * v_y)) * 4.0f;
                    (*vtx)->v.ob[2] = -0x14;
                    (*vtx)->v.tc[0] = ((texture_width - 1) * v_x) << 6;
                    (*vtx)->v.tc[1] = ((texture_height - 1) * v_y) << 6;
                    (*vtx)++;
                }
            }
            gSP1Quadrangle((*gfx)++, 0, 1, 3, 2, 0);

            // Clear the matrix group
            gEXPopMatrixGroup((*gfx)++, G_MTX_MODELVIEW);
        }

        gDPPipeSync((*gfx)++);
        gDPSetTextureLUT((*gfx)++, G_TT_NONE);
        gDPPipelineMode((*gfx)++, G_PM_NPRIMITIVE);
        viewport_setRenderViewportAndPerspectiveMatrix(gfx, mtx);
    }
}

// Override BanjoRecomp's level-specific item count (presents/acorns/caterpillars) with no scissor setup
RECOMP_FORCE_PATCH void fxcommon3score_draw(enum item_e item_id, void *arg1, Gfx **gfx, Mtx **mtx, Vtx **vtx) {
    extern s32 itemPrint_getValue(s32 item_id);
    extern s32 func_802FB0D4(void *arg1);
    extern f32 viewport_transformCoordinate(f32, f32, f32 *, f32 *);
    extern void func_8033A308(f32 *);
    extern void anctrl_drawSetup(AnimCtrl *ctrl, f32 *pos, s32 arg2);
    extern void func_80253208(Gfx **gdl, s32 x, s32 y, s32 w, s32 h, void *color_buffer);
    extern f32 vtxList_getGlobalNorm(BKVertexList *);
    extern BKVertexList *model_getVtxList(BKModelBin *);
    extern s32 getGameMode(void);
    extern s32 getActiveFramebuffer(void);
    extern f32 func_802FB0E4(struct8s *);

    typedef struct {
        u8 pad0[0x14];
        s32 unk14;
        u8 pad18[0x8];
        s32 item_id;
        s32 model_id;
        s32 anim_id;
        f32 anim_duration;
        f32 unk30;
        f32 unk34;
        f32 unk38;
        f32 unk3C;
        f32 unk40;
        f32 unk44;
        f32 unk48;
        f32 unk4C;
        f32 unk50;
        f32 unk54;
        BKModelBin *model;
        char value_string[0xC];
        f32 unk68;
        f32 unk6C;
        AnimCtrl *anim_ctrl;
    } Struct_core2_79830_0;

    Struct_core2_79830_0 *a1 = (Struct_core2_79830_0 *)arg1;
    f32 sp68[3];
    f32 sp5C[3];
    f32 sp50[3];
    f32 sp44[3];
    f32 sp40;
    f32 sp3C;
    
    sp40 = func_802FB0E4(arg1) * a1->unk54 + a1->unk34;
    if (a1->model != NULL && func_802FB0D4(arg1)) {
        a1->value_string[0] = '\0';
        strIToA(a1->value_string, itemPrint_getValue(item_id));
        print_bold_spaced(a1->unk30 + a1->unk40, sp40 + a1->unk44, a1->value_string);
        sp3C = viewport_transformCoordinate(a1->unk30, sp40, sp5C, sp68);

        sp44[0] = 0.0f;
        sp44[1] = a1->unk38;
        sp44[2] = 0.0f;

        sp50[0] = 0.0f;
        sp50[1] = a1->unk68;
        sp50[2] = 0.0f;
        func_8033A308(sp50);
        if (getGameMode() != GAME_MODE_4_PAUSED) {
            modelRender_setDepthMode(MODEL_RENDER_DEPTH_FULL);
        }
        sp68[0] += a1->unk4C;
        if (a1->unk6C == 0.0f) {
            a1->unk6C = 1.1 * (vtxList_getGlobalNorm(model_getVtxList(a1->model)) * a1->unk3C);
        }
        func_80253208(gfx, a1->unk30 - a1->unk6C, sp40 - a1->unk6C, 2 * a1->unk6C, 2 * a1->unk6C, gFramebuffers[getActiveFramebuffer()]);
        if (a1->anim_ctrl != NULL) {
            anctrl_drawSetup(a1->anim_ctrl, sp5C, 1);
        }
        modelRender_draw(gfx, mtx, sp5C, sp68, a1->unk3C * sp3C, sp44, a1->model);
    }
}

// Override BanjoRecomp's item count function with no scissor setup
RECOMP_FORCE_PATCH void fxcommon2score_draw(enum item_e item_id, struct8s *arg1, Gfx **gfx, Mtx **mtx, Vtx **vtx) {
    extern s32 itemPrint_getValue(s32 item_id);
    extern s32 level_get(void);
    extern s32 itemscore_noteScores_getTotal(void);
    extern void func_802FD360(struct8s *arg0, Gfx **gfx, Mtx **mtx, Vtx **vtx);
    extern f32 func_802FB0DC(struct8s *this);
    extern f32 func_802FB0E4(struct8s *this);

    f32 pad;
    s32 sp38;
    f32 sp34;

    sp38 = itemPrint_getValue(item_id);
    sp34 = 0.0f;
    if (item_id == ITEM_C_NOTE) {
        if (level_get() == LEVEL_6_LAIR || level_get() == LEVEL_C_BOSS) {
            sp38 = itemscore_noteScores_getTotal();
        }
    }
    if (item_id < 6) {
        sp38 = ((sp38) ? 1 : 0) + sp38 / 60;
    }
    if (item_id == ITEM_1B_VILE_VILE_SCORE && 9 < sp38) {
        sp34 = -16.0f;
    }
    if (item_id == ITEM_1C_MUMBO_TOKEN || item_id == ITEM_25_MUMBO_TOKEN_TOTAL) {
        if (sp38 >= 100) {
            sp38 = 99;
        }
    }
    arg1->string_54[0] = 0;
    strIToA(arg1->string_54, sp38);

    print_bold_spaced(
        (s32)(func_802FB0DC(arg1) + arg1->unk38 + arg1->unk44 + sp34),
        (s32)(func_802FB0E4(arg1) * arg1->unk4C + (arg1->unk3C + arg1->unk48)),
        arg1->string_54
    );

    func_802FD360(arg1, gfx, mtx, vtx);
}


// Override BanjoRecomp's life counter with no scissor setup
RECOMP_FORCE_PATCH void fxlifescore_draw(enum item_e item_id, struct8s *arg1, Gfx **gfx, Mtx **mtx, Vtx **vtx) {
    extern void *D_80381EB0[];
    extern s32 D_80381EC4;
    extern f32 D_80381EB8;
    extern f32 D_80381EBC;
    extern char code_78E50_ItemValueString[8];
    extern Gfx D_8036A278[];
    extern void func_80348044(Gfx **gfx, BKSprite *sprite, s32 frame, s32 tmem, s32 rtile, s32 uls, s32 ult, s32 cms, s32 cmt, s32 *width, s32 *height, s32 *frame_width, s32 *frame_height, s32 *texture_x, s32 *texture_y, s32 *textureCount);
    extern s32 itemPrint_getValue(s32 item_id);
    extern f32 func_802FB0E4(struct8s *this);

    s32 sp10C;
    Vtx *sp108;
    s32 sp104;
    s32 var_v0;
    s32 var_v1;
    s32 var_s5;
    s32 var_s4;
    s32 spF0;
    s32 spEC;
    s32 spE8;
    s32 spE4;
    s32 spE0;
    s32 spDC;

    sp10C = -1;
    sp108 = *vtx;
    code_78E50_ItemValueString[0] = '\0';
    strIToA(code_78E50_ItemValueString, MIN(9, itemPrint_getValue(item_id)));

    print_bold_spaced(0x4E, (s32)(func_802FB0E4(arg1) + -16.0f + 4.0f), (char *)&code_78E50_ItemValueString);

    if (1); 
    if (D_80381EB0[D_80381EC4] != NULL) {
        gSPDisplayList((*gfx)++, D_8036A278);
        viewport_setRenderViewportAndOrthoMatrix(gfx, mtx);
        if (gfx);

        gDPPipeSync((*gfx)++);
        gDPSetCombineLERP((*gfx)++, 0, 0, 0, TEXEL0, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, TEXEL0, TEXEL0, 0, PRIMITIVE, 0);
        gDPSetPrimColor((*gfx)++, 0, 0, 0x00, 0x00, 0x00, 0xFF);
        do {
            func_80348044(gfx, D_80381EB0[D_80381EC4], (s32)D_80381EBC % 4, 0, 0, 0, 0, 2, 2, &spF0, &spEC, &spE8, &spE4, &spE0, &spDC, &sp10C);

            if (((*vtx - sp108) & 0xF) == 0) {
                gSPVertex((*gfx)++, *vtx, MIN(0x10, (1 + sp10C) << 2), 0);
                sp104 = 0;
            }
            else {
                sp104 = sp104 + 4;
            }

            var_s5 = (40.0f - ((f32)gFramebufferWidth / 2)) + spE0;
            var_s4 = (((((f32)gFramebufferHeight / 2) - func_802FB0E4(arg1)) - -16.0f) - spDC);

            for (var_v1 = 0; var_v1 < 2; var_v1++) {
                for (var_v0 = 0; var_v0 < 2; var_v0++) {
                    (*vtx)->v.ob[0] = (s16)(s32)(((((f32)spF0 * D_80381EB8 * (f32)var_v0) - (((f32)spE8 * D_80381EB8) / 2)) + var_s5) * 4.0f);
                    (*vtx)->v.ob[1] = (s16)(s32)((((((f32)spE4 * D_80381EB8) / 2) - ((f32)spEC * D_80381EB8 * var_v1)) + var_s4) * 4.0f);
                    (*vtx)->v.ob[2] = -0x14;
                    (*vtx)->v.tc[0] = ((spF0 - 1) * var_v0) << 6;
                    (*vtx)->v.tc[1] = ((spEC - 1) * var_v1) << 6;
                    (*vtx)++;
                }
            }
            gSP1Quadrangle((*gfx)++, sp104, sp104 + 1, sp104 + 3, sp104 + 2, 0);
        } while (sp10C != 0);

        gDPPipeSync((*gfx)++);
        gDPSetTextureLUT((*gfx)++, G_TT_NONE);
        gDPPipelineMode((*gfx)++, G_PM_NPRIMITIVE);
        viewport_setRenderViewportAndPerspectiveMatrix(gfx, mtx);
    }
}


// Override BanjoRecomp's healthbar with no scissor setup
RECOMP_FORCE_PATCH void fxhealthscore_draw(enum item_e item_id, struct8s *arg1, Gfx **gfx, Mtx **mtx, Vtx **vtx) {
    extern BKSprite *gSpriteHealth;
    extern BKSprite *gSpriteRedHealth;
    extern Gfx D_8036A918[];
    extern s32 gTotalHealth;
    extern f32 gHealth;
    extern f32 D_80381EFC;
    extern f32 D_80381F08[];
    extern void func_80347FC0(Gfx **gfx, BKSprite *sprite, s32 frame, s32 tmem, s32 rtile, s32 uls, s32 ult, s32 cms, s32 cmt, s32 *width, s32 *height);
    extern f32 func_802FB0E4(struct8s *this);

    int i;
    int tmp_v1;
    s32 honeycomb_width;
    s32 honeycomb_height;
    int tmp_v0;
    f32 f18;
    f32 f14;
    f32 f20;
    s32 is_red_health_initialized = FALSE;
    s32 s6;

    if (gSpriteHealth == NULL) {
        return;
    }

    gSPDisplayList((*gfx)++, D_8036A918);
    func_80347FC0(gfx, gSpriteHealth, 0, 0, 0, 0, 0, 2, 2, &honeycomb_width, &honeycomb_height);
    viewport_setRenderViewportAndOrthoMatrix(gfx, mtx);

    for (i = gTotalHealth - 1; i >= 0; i--) {
        if (i != 0 && (i + 1 != gTotalHealth || gTotalHealth & 1)) {
            s6 = (i & 1) ? i + 1 : i - 1;
        }
        else {
            s6 = i;
        }

        gDPPipeSync((*gfx)++);

        if (gHealth > i) {
            if (0 < (gHealth - 8.0f) && (gHealth - 8.0f) > i) {
                if (!is_red_health_initialized) {
                    func_80347FC0(gfx, gSpriteRedHealth, 0, 0, 0, 0, 0, 2, 2, &honeycomb_width, &honeycomb_height);
                    is_red_health_initialized = TRUE;
                }
            }

            gDPSetPrimColor((*gfx)++, 0, 0, 0xFF, 0xFF, 0xFF, 0xFF);
        }
        else {
            gDPSetPrimColor((*gfx)++, 0, 0, 0xFF, 0xFF, 0xFF, 0x78);
        }

        f20 = 96.0f - (f32)gFramebufferWidth / 2 + (i * 13);
        f14 = (f32)gFramebufferHeight / 2 - func_802FB0E4(arg1) - D_80381F08[s6] - -48.0f;
        f14 = (i & 1) ? f14 + 5.75 : f14 - 5.75;

        gSPVertex((*gfx)++, *vtx, 4, 0);

        for (tmp_v1 = 0; tmp_v1 < 2; tmp_v1++) {
            for (tmp_v0 = 0; tmp_v0 < 2; tmp_v0++) {
                (*vtx)->v.ob[0] = (((honeycomb_width * D_80381EFC) * tmp_v0 - (honeycomb_width * D_80381EFC) / 2) + f20) * 4.0f;
                (*vtx)->v.ob[1] = (((honeycomb_height * D_80381EFC) / 2 - (honeycomb_height * D_80381EFC) * tmp_v1) + f14) * 4.0f;
                (*vtx)->v.ob[2] = -0x14;

                (*vtx)->v.tc[0] = ((honeycomb_width - 1) * tmp_v0) << 6;
                (*vtx)->v.tc[1] = ((honeycomb_height - 1) * tmp_v1) << 6;
                (*vtx)++;
            }
        }

        gSP1Quadrangle((*gfx)++, 0, 1, 3, 2, 0);
    }

    gDPPipeSync((*gfx)++);
    gDPSetTextureLUT((*gfx)++, G_TT_NONE);
    gDPPipelineMode((*gfx)++, G_PM_NPRIMITIVE);
    viewport_setRenderViewportAndPerspectiveMatrix(gfx, mtx);
}


// Override BanjoRecomp's Jinjo score with no scissor setup
RECOMP_FORCE_PATCH void fxjinjoscore_draw(s32 arg0, struct8s *arg1, Gfx **gfx, Mtx **mtx, Vtx **vtx) {
    extern BKSprite *D_80381E40[];
    extern u8 D_80381E58[];
    extern f32 D_80381E54;
    extern f32 D_80381E60[];
    extern f32 D_80381E78[];
    extern u16 D_80381620[][5][0x10];
    extern Gfx D_8036A228[];
    extern void func_80347FC0(Gfx **gfx, BKSprite *sprite, s32 frame, s32 tmem, s32 rtile, s32 uls, s32 ult, s32 cms, s32 cmt, s32 *width, s32 *height);
    extern f32 func_802FB0E4(struct8s *this);

    BKSprite *sprite;
    s32 draw_index;
    s32 texture_width;
    s32 texture_height;
    s32 jinjo_id;
    f32 center_y;
    f32 center_x;
    f32 x_offset;
    f32 y_offset;
    f32 pos_x;
    s32 i;
    s32 j;

    gSPDisplayList((*gfx)++, D_8036A228);
    viewport_setRenderViewportAndOrthoMatrix(gfx, mtx);
    pos_x = 44.0f;
    
    for(jinjo_id = 0; jinjo_id < 5; jinjo_id++){
        s32 jinjo_collected;
        sprite = D_80381E40[jinjo_id];
        jinjo_collected = (D_80381E58[jinjo_id] != 0) ? 1 : 0;
        if (sprite != NULL) {
            func_80347FC0(gfx, sprite, (s32)D_80381E60[jinjo_id], 0, 0, 0, 0, 2, 2, &texture_width, &texture_height);
            gDPLoadTLUT_pal16((*gfx)++, 0, D_80381620[(s32)D_80381E60[jinjo_id]][jinjo_id]);
            x_offset = 0.0f;
            y_offset = 0.0f;
            
            for (draw_index = jinjo_collected; draw_index >= 0; draw_index--){
                gDPPipeSync((*gfx)++);
                if (draw_index != 0) {
                    gDPSetCombineLERP((*gfx)++, 0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE, 0);
                    gDPSetPrimColor((*gfx)++, 0, 0, 0x00, 0x00, 0x00, 0x8C);
                } else {
                    gDPSetCombineLERP((*gfx)++, 0, 0, 0, TEXEL0, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, TEXEL0, TEXEL0, 0, PRIMITIVE, 0);
                    gDPSetPrimColor((*gfx)++, 0, 0, 0x00, 0x00, 0x00, jinjo_collected ? 0xFF : 0x6E);
                }
                center_x = pos_x - (f32)gFramebufferWidth / 2 + x_offset;
                center_y = (f32)gFramebufferHeight / 2 + func_802FB0E4(arg1) - 266.0f + 40.0f + y_offset - D_80381E78[jinjo_id];
                gSPVertex((*gfx)++, *vtx, 4, 0);
                
                for(i = 0; i < 2; i++){
                    for(j = 0; j < 2; j++){
                        (*vtx)->v.ob[0] = ((texture_width * D_80381E54 * j) - (texture_width * D_80381E54 / 2) + center_x) * 4;
                        (*vtx)->v.ob[1] = ((texture_height * D_80381E54 / 2) - (texture_height * D_80381E54 * i) + center_y) * 4;
                        (*vtx)->v.ob[2] = -20;
                        (*vtx)->v.tc[0] = ((texture_width - 1) * j) << 6;
                        (*vtx)->v.tc[1] = ((texture_height - 1) * i) << 6;
                        (*vtx)++;
                    }
                }
                gSP1Quadrangle((*gfx)++, 0, 1, 3, 2, 0);
                x_offset += -2;
                y_offset += 2;
            }
        }
        pos_x += 32.0f;
    }
    gDPPipeSync((*gfx)++);
    gDPSetTextureLUT((*gfx)++, G_TT_NONE);
    gDPPipelineMode((*gfx)++, G_PM_NPRIMITIVE);
    viewport_setRenderViewportAndPerspectiveMatrix(gfx, mtx);
}


// Patch the zoombox text draw function to align pause menu text to the left side of the
// 16:9 screen. func_803162B4 has no Gfx** so it can't set a scissor directly.
RECOMP_FORCE_PATCH void func_803162B4(GcZoombox *this) {
    extern void func_802F7B90(s32 arg0, s32 arg1, s32 arg2);
    extern void func_802F79D0(s32 arg0, s32 arg1, u8 *str, s32 arg3, s32 arg4);
    extern void print_bold_spaced(s32 x, s32 y, u8 *str);
    extern void print_dialog(s32 x, s32 y, u8 *str);

    func_802F7B90(this->unk168, this->unk168, this->unk168);
    if (this->unk1A4_30) {
        if (this->unk1A4_17) {
            func_802F79D0(this->unk16A, this->unk16C, this->unk0, this->unk166, -1);
        }
        else if (this->unk1A4_15) {
            print_bold_spaced(this->unk16A, this->unk16C, this->unk0);
        }
        else {
            print_dialog(this->unk16A, this->unk16C, this->unk0);
        }
    }
    if (this->unk1A4_29) {
        if (this->unk1A4_15) {
            print_bold_spaced(this->unk16A, this->unk16E, this->unk30);
        }
        else {
            print_dialog(this->unk16A, this->unk16E, this->unk30);
        }
    }
    func_802F7B90(0xff, 0xff, 0xff);
}

// Override zoombox sprite rendering to remove scissor setup
RECOMP_FORCE_PATCH void func_803164B0(GcZoombox *this, Gfx **gfx, Mtx **mtx, s32 arg3, s32 arg4, BKSpriteDisplayData *arg5, f32 arg6) {
    extern void func_80338338(s32, s32, s32);
    extern void func_803382FC(u8);
    extern void func_803382E4(s32);
    extern void func_80335D30(Gfx **);
    extern void mlMtxIdent(void);
    extern void mlMtxRotYaw(f32);
    extern void func_80252330(f32, f32, f32);
    extern void mlMtxScale_xyz(f32, f32, f32);
    extern void mlMtxApply(Mtx *);
    extern void func_80344090(BKSpriteDisplayData *, s32, Gfx **);
    extern void func_8033687C(Gfx **);
    
    f32 sp2C[3];
    f32 temp_f12;

    if (this->portrait_id == 46) {
        arg6 = 0.75f;
    }
    func_80338338(0xFF, 0xFF, 0xFF);
    func_803382FC(this->unk168 * arg6);
    func_803382E4(5);
    func_80335D30(gfx);
    
    // Remove scissor setup - just call viewport function directly
    viewport_setRenderViewportAndOrthoMatrix(gfx, mtx);
    
    mlMtxIdent();
    if (this->unk1A4_24) {
        mlMtxRotYaw(180.0f);
        sp2C[0] = (f32) this->unk170 - ((f32) arg3 * this->unk198);
    } else {
        sp2C[0] = (f32) this->unk170 + ((f32) arg3 * this->unk198);
    }
    sp2C[1] = this->unk172 + ((f32) arg4 * this->unk198);
    sp2C[2] = -10.0f;
    func_80252330((sp2C[0] * 4.0f) - ((f32)gFramebufferWidth * 2), ((f32)gFramebufferHeight * 2) - (sp2C[1] * 4.0f), sp2C[2]);
    temp_f12 = (f32) ((f64) this->unk198 * 0.8);
    mlMtxScale_xyz(temp_f12, temp_f12, 1.0f);
    mlMtxApply(*mtx);
    gSPMatrix((*gfx)++, (*mtx)++, G_MTX_LOAD | G_MTX_MODELVIEW);
    modelRender_setDepthMode(MODEL_RENDER_DEPTH_NONE);
    func_80344090(arg5, this->unk186, gfx);
    func_8033687C(gfx);
    viewport_setRenderViewportAndPerspectiveMatrix(gfx, mtx);
}

// Override extra honeycomb HUD to remove scissor setup
RECOMP_FORCE_PATCH void fxhoneycarrierscore_draw(s32 arg0, struct8s *arg1, Gfx **arg2, Mtx **arg3, Vtx **arg4) {
    extern void *D_8036A010;
    extern void *D_8036A014;
    extern s32 D_8036A018[];
    extern Gfx D_8036A030[];
    extern s32 D_803815C0;
    extern f32 D_803815CC;
    extern f32 D_803815C8;
    extern f32 D_803815D0;
    extern f32 D_803815D4;
    extern f32 D_803815D8;
    extern f32 D_803815DC;
    extern f32 D_803815E0;
    extern s32 D_803815E4;
    extern s32 D_803815E8;
    extern s32 D_803815EC;
    extern void func_80347FC0(Gfx **gfx, BKSprite *sprite, s32 frame, s32 tmem, s32 rtile, s32 uls, s32 ult, s32 cms, s32 cmt, s32 *width, s32 *height);
    extern f32 func_802FB0E4(struct8s *this);
    extern f32 func_802FDE60(f32);
    
    f64 var_f24;
    s32 sp13C;
    s32 sp138;
    s32 sp134;
    f32 sp130;
    f32 sp12C;
    f32 sp128;
    f32 sp124;
    s32 var_v0;
    s32 var_v1;
    u32 sp118;
    f32 pad;
    f32 sp110;

    sp118 = D_803815C0 == 2;
    if (D_8036A010 != 0) {
        func_80347FC0(arg2, (sp118) ? (D_8036A014 != 0) ? D_8036A014 : D_8036A010 : D_8036A010, 0, 0, 0, 0, 0, 2, 2, &sp13C, &sp138);
        
        // Remove scissor setup - just call viewport function directly
        viewport_setRenderViewportAndOrthoMatrix(arg2, arg3);
        
        gSPDisplayList((*arg2)++, D_8036A030);
        for(sp134 = 0; sp134 < ((sp118)? ((D_8036A014 != 0) ? 2 : 1) : 6); sp134++){
            sp110 = D_8036A018[sp134] * -0x3C;
            gDPPipeSync((*arg2)++);
            if (sp118) {
                if (sp134 != 0) {
                    func_80347FC0(arg2, D_8036A010, 0, 0, 0, 0, 0, 2, 2, &sp13C, &sp138);
                    gDPSetPrimColor((*arg2)++, 0, 0, 0x00, 0x00, 0x00, (0xFF - D_803815E4));
                } else {
                    gDPSetPrimColor((*arg2)++, 0, 0, 0x00, 0x00, 0x00, D_803815E4);
                }
            } else {
                if (D_803815D4 <= D_8036A018[sp134]) {
                    gDPSetPrimColor((*arg2)++, 0, 0, 0x00, 0x00, 0x00, 0x50);
                }
                else{
                    if ((D_803815EC != 0) && ((D_803815D4 - 1.0f) == D_8036A018[sp134])) {
                        gDPSetPrimColor((*arg2)++, 0, 0, 0x00, 0x00, 0x00, D_803815E8);
                    } else {
                        gDPSetPrimColor((*arg2)++, 0, 0, 0x00, 0x00, 0x00, 0xFF);
                    }
                }
            }
            sp128 = (244.0f - ((f32) gFramebufferWidth / 2));
            sp124 = func_802FB0E4(arg1) + ((f32) gFramebufferHeight / 2) - 246.0f;
            guTranslate(*arg3, sp128 * 4.0f, sp124 * 4.0f, 0.0f);
            gSPMatrix((*arg2)++, OS_K0_TO_PHYSICAL((*arg3)++), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            guRotate(*arg3, func_802FDE60(D_803815D8 + D_803815DC), 0.0f, 0.0f, 1.0f);
            gSPMatrix((*arg2)++, OS_K0_TO_PHYSICAL((*arg3)++), G_MTX_NOPUSH | G_MTX_MUL | G_MTX_MODELVIEW);
            guScale(*arg3, D_803815E0, D_803815E0, D_803815E0);
            gSPMatrix((*arg2)++, OS_K0_TO_PHYSICAL((*arg3)++), G_MTX_NOPUSH | G_MTX_MUL | G_MTX_MODELVIEW);
            guTranslate(*arg3, -sp128 * 4.0f, -sp124 * 4.0f, 0.0f);
            gSPMatrix((*arg2)++, OS_K0_TO_PHYSICAL((*arg3)++), G_MTX_NOPUSH | G_MTX_MUL | G_MTX_MODELVIEW);
            var_f24 = MIN(1.0, MAX(0.0, D_803815C8));
            sp130 = cosf(((D_803815CC + sp110) * 0.017453292519943295)) * (var_f24 * 24.5) * D_803815D0;
            var_f24 = MIN(1.0, MAX(0.0, D_803815C8));
            sp12C = sinf(((D_803815CC + sp110) * 0.017453292519943295))* (var_f24 * 24.5) * D_803815D0;
            gSPVertex((*arg2)++, *arg4, 4, 0);
            for(var_v1 = 0; var_v1 < 2; var_v1++){
                for(var_v0 = 0; var_v0 < 2; var_v0++, (*arg4)++){
                    (*arg4)->v.ob[0] = ((((sp13C * D_803815D0) * var_v0) - ((sp13C * D_803815D0) / 2)) + (s32) (sp130 + sp128)) * 4.0f;
                    (*arg4)->v.ob[1] = ((((sp138 * D_803815D0) / 2) - ((sp138 * D_803815D0) * var_v1)) + (s32) (sp12C + sp124)) * 4.0f;
                    (*arg4)->v.ob[2] = -0x14;
                    (*arg4)->v.tc[0] = (s16) ((sp13C - 1) * var_v0 << 9);
                    (*arg4)->v.tc[1] = (s16) ((sp138 - 1) * var_v1 << 9);
                }
            }
            gSP1Quadrangle((*arg2)++, 0, 1, 3, 2, 0);
        }
        gDPPipeSync((*arg2)++);
        gDPSetTextureLUT((*arg2)++, G_TT_NONE);
        gDPPipelineMode((*arg2)++, G_PM_NPRIMITIVE);
        viewport_setRenderViewportAndPerspectiveMatrix(arg2, arg3);
    }
}
