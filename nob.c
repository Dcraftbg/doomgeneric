#define NOB_STRIP_PREFIX
#define NOB_IMPLEMENTATION
#include "nob.h"

const char* generic_sources[] = {
    "dummy.c",
    "am_map.c",
    "doomdef.c",
    "doomstat.c",
    "dstrings.c",
    "d_event.c",
    "d_items.c",
    "d_iwad.c",
    "d_loop.c",
    "d_main.c",
    "d_mode.c",
    "d_net.c",
    "f_finale.c",
    "f_wipe.c",
    "g_game.c",
    "hu_lib.c",
    "hu_stuff.c",
    "info.c",
    "i_cdmus.c",
    "i_endoom.c",
    "i_joystick.c",
    "i_scale.c",
    "i_sound.c",
    "i_system.c",
    "i_timer.c",
    "memio.c",
    "m_argv.c",
    "m_bbox.c",
    "m_cheat.c",
    "m_config.c",
    "m_controls.c",
    "m_fixed.c",
    "m_menu.c",
    "m_misc.c",
    "m_random.c",
    "p_ceilng.c",
    "p_doors.c",
    "p_enemy.c",
    "p_floor.c",
    "p_inter.c",
    "p_lights.c",
    "p_map.c",
    "p_maputl.c",
    "p_mobj.c",
    "p_plats.c",
    "p_pspr.c",
    "p_saveg.c",
    "p_setup.c",
    "p_sight.c",
    "p_spec.c",
    "p_switch.c",
    "p_telept.c",
    "p_tick.c",
    "p_user.c",
    "r_bsp.c",
    "r_data.c",
    "r_draw.c",
    "r_main.c",
    "r_plane.c",
    "r_segs.c",
    "r_sky.c",
    "r_things.c",
    "sha1.c",
    "sounds.c",
    "statdump.c",
    "st_lib.c",
    "st_stuff.c",
    "s_sound.c",
    "tables.c",
    "v_video.c",
    "wi_stuff.c",
    "w_checksum.c",
    "w_file.c",
    "w_main.c",
    "w_wad.c",
    "z_zone.c",
    "w_file_stdc.c",
    "i_input.c",
    "i_video.c",
    "doomgeneric.c",
    "mus2mid.c",
};
int main(int argc, char** argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);
    const char* cc = getenv("CC");
    if(!cc) {
        cc = 
#ifdef _WIN32
            "clang"
#else
            "cc"
#endif
        ;
    }
    const char* bindir = getenv("BINDIR");
    if(!bindir) bindir = ".build";
    if(!mkdir_if_not_exists(bindir)) return 1;
    if(!mkdir_if_not_exists(temp_sprintf("%s/doomgeneric", bindir))) return 1;

    File_Paths sources = { 0 };
    da_append_many(&sources, generic_sources, ARRAY_LEN(generic_sources));
    // TODO: Other targets
    const char* minos_root = getenv("MINOSROOT");
    if(!minos_root) {
        nob_log(NOB_ERROR, "Missing $MINOSROOT!");
        return 1;
    }
    da_append(&sources, "doomgeneric_minos.c");

    File_Paths pathb = { 0 };
    String_Builder stb = { 0 };
    Cmd cmd = { 0 };
    File_Paths objs = { 0 };
    for(size_t i = 0; i < sources.count; ++i) {
        const char* src = temp_sprintf("doomgeneric/%s", sources.items[i]);
        const char* obj = temp_sprintf("%s/doomgeneric/%.*s.o", bindir, (int)strlen(sources.items[i]) - 2, sources.items[i]);
        da_append(&objs, obj);
        if(nob_c_needs_rebuild1(&stb, &pathb, obj, src)) {
            cmd_append(&cmd, cc);
            cmd_append(&cmd, "-O1", "-c");
            // debug symbols
#if 0
            cmd_append(&cmd, "-g", "-ggdb");
#endif
            // TODO: Other targets
            cmd_append(&cmd, "-MD");
            cmd_append(&cmd, "-I../libpluto/include", "-I../libwm/include");
            cmd_append(&cmd, src, "-o", obj);
            if(!cmd_run_sync_and_reset(&cmd)) return 1;
        }
    }
    const char* output = temp_sprintf("%s/doomgeneric/doomgeneric", bindir);
    if(needs_rebuild(output, objs.items, objs.count)) {
        cmd_append(&cmd, cc);
        da_append_many(&cmd, objs.items, objs.count);
        cmd_append(&cmd, "-o", output);
        cmd_append(&cmd, temp_sprintf("-L%s/bin/libpluto", minos_root), temp_sprintf("-L%s/bin/libwm", minos_root), "-lpluto", "-lwm");
        if(!cmd_run_sync_and_reset(&cmd)) return 1;
    }
    char* rootdir = getenv("ROOTDIR");
    const char* copyto = temp_sprintf("%s/user/doomgeneric", rootdir);
    if(rootdir && needs_rebuild1(copyto, output) && !copy_file(output, copyto)) 
        return 1;
}
