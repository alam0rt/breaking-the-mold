// Apply evidence-based field names/types uncovered during the audit.
// Data-driven: edit FIELDS below and re-run. Run via plain-install
// analyzeHeadless (see ApplyAuditFixes.java header). Idempotent.
//@category Skullmonkeys
import ghidra.app.script.GhidraScript;
import ghidra.program.model.data.*;

public class UncoverFields extends GhidraScript {

    // Embedded /btm/SpriteContext (0x14) overlays entity+0x78. Anchors:
    // frame_metadata@0x78=pFrameTable, total_frame_count@0x88=frameCount,
    // spriteLookupByte@0x8a=texPageByte, is_valid@0x8b=spriteContextValid.
    // => SpriteContext.rle_data/max_width/max_height fill the 0x80-0x87 gap.

    @Override
    public void run() throws Exception {
        DataTypeManager dtm = currentProgram.getDataTypeManager();
        String[] structs = {
            "/Skullmonkeys/SpriteEntity", "/Skullmonkeys/PlayerEntity",
            "/Skullmonkeys/FinnPlayerEntity", "/Skullmonkeys/RunnPlayerEntity",
            "/Skullmonkeys/SoarPlayerEntity", "/Skullmonkeys/PathEnemyEntity",
        };
        int tx = currentProgram.startTransaction("audit: uncover embedded SpriteContext fields");
        boolean ok = false;
        try {
            for (String s : structs) {
                set(dtm, s, 0x80, new PointerDataType(), 4, "pRleData",
                    "Embedded SpriteContext+0x08: RLE pixel data base");
                set(dtm, s, 0x84, UnsignedShortDataType.dataType, 2, "spriteMaxWidth",
                    "Embedded SpriteContext+0x0c: max width across frames");
                set(dtm, s, 0x86, UnsignedShortDataType.dataType, 2, "spriteMaxHeight",
                    "Embedded SpriteContext+0x0e: max height across frames");
            }
            // data-type cleanup: meaningful SpriteRenderContext words still typed undefined2
            set(dtm, "/SpriteRenderContext", 0x08, UnsignedShortDataType.dataType, 2, "wSlotId",
                "VRAM slot id");
            set(dtm, "/SpriteRenderContext", 0x24, UnsignedShortDataType.dataType, 2, "wTpage",
                "GPU texture page word");
            set(dtm, "/SpriteRenderContext", 0x26, UnsignedShortDataType.dataType, 2, "wClut",
                "GPU CLUT word");
            ok = true;
        } finally {
            currentProgram.endTransaction(tx, ok);
        }
        println("UncoverFields done (committed=" + ok + ")");
    }

    private void set(DataTypeManager dtm, String path, int off, DataType dt, int len,
                     String name, String comment) {
        Structure s = (Structure) dtm.getDataType(path);
        if (s == null) { println("!! missing " + path); return; }
        DataTypeComponent cur = s.getComponentContaining(off);
        String old = cur == null ? "(none)" : cur.getDataType().getName() + " " + cur.getFieldName();
        s.replaceAtOffset(off, dt, len, name, comment);
        println("SET " + path + "+0x" + Integer.toHexString(off) + ": " + old + " -> " + dt.getName() + " " + name);
    }
}
