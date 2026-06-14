// Create the enums that the original Skullmonkeys source very likely had, and
// apply them to the obvious fields. Reveals intent in the decompiler.
//   EntityType       (from the 121-entry callback table @ 0x8009D5F8)
//   AnimChangeFlags  (animChangeFlags bitfield; SetEntitySpriteId ORs 0x1FC)
//   PadButton        (PSX libetc PadRead() masks; matches InputState usage)
// Run via plain-install analyzeHeadless (see ApplyAuditFixes.java header).
//@category Skullmonkeys
import ghidra.app.script.GhidraScript;
import ghidra.program.model.data.*;
import java.nio.file.*;
import java.util.List;

public class CreateAuditEnums extends GhidraScript {

    @Override
    public void run() throws Exception {
        DataTypeManager dtm = currentProgram.getDataTypeManager();
        CategoryPath sk = new CategoryPath("/Skullmonkeys");
        int tx = currentProgram.startTransaction("audit: create + apply enums");
        boolean ok = false;
        try {
            // --- EntityType (2 bytes) from generated TSV ---
            EnumDataType et = new EnumDataType(sk, "EntityType", 2, dtm);
            int n = 0;
            for (String line : Files.readAllLines(Paths.get("/tmp/enum_entitytype.tsv"))) {
                if (line.isBlank()) continue;
                String[] p = line.split("\t");
                et.add(p[1].trim(), Long.parseLong(p[0].trim()));
                n++;
            }
            DataType etR = dtm.addDataType(et, DataTypeConflictHandler.REPLACE_HANDLER);
            println("EntityType enum: " + n + " members");

            // --- AnimChangeFlags (2 bytes, flag enum) ---
            EnumDataType af = new EnumDataType(sk, "AnimChangeFlags", 2, dtm);
            af.add("ANIM_CHG_SPRITE", 0x004);
            af.add("ANIM_CHG_FRAME", 0x008);
            af.add("ANIM_CHG_LOOP_FRAME", 0x010);
            af.add("ANIM_CHG_TARGET_FRAME", 0x020);
            af.add("ANIM_CHG_DIRECTION", 0x040);
            af.add("ANIM_CHG_LOOP_FLAG", 0x080);
            af.add("ANIM_CHG_ANIM_ACTIVE", 0x100);
            af.add("ANIM_CHG_TARGET_BY_VALUE", 0x800);
            DataType afR = dtm.addDataType(af, DataTypeConflictHandler.REPLACE_HANDLER);
            println("AnimChangeFlags enum: 8 members");

            // --- PadButton (2 bytes, flag enum) — libetc PadRead() layout ---
            EnumDataType pb = new EnumDataType(sk, "PadButton", 2, dtm);
            pb.add("PAD_L2", 0x0001);
            pb.add("PAD_R2", 0x0002);
            pb.add("PAD_L1", 0x0004);
            pb.add("PAD_R1", 0x0008);
            pb.add("PAD_TRIANGLE", 0x0010);
            pb.add("PAD_CIRCLE", 0x0020);
            pb.add("PAD_CROSS", 0x0040);
            pb.add("PAD_SQUARE", 0x0080);
            pb.add("PAD_SELECT", 0x0100);
            pb.add("PAD_START", 0x0800);
            pb.add("PAD_UP", 0x1000);
            pb.add("PAD_RIGHT", 0x2000);
            pb.add("PAD_DOWN", 0x4000);
            pb.add("PAD_LEFT", 0x8000);
            DataType pbR = dtm.addDataType(pb, DataTypeConflictHandler.REPLACE_HANDLER);
            println("PadButton enum: 14 members");

            // --- apply to fields ---
            applyEnum(dtm, "/Skullmonkeys/EntityDefinition", 0x12, etR);
            applyEnum(dtm, "/Skullmonkeys/InputState", 0x00, pbR);
            String[] spriteStructs = {
                "/Skullmonkeys/SpriteEntity", "/Skullmonkeys/PlayerEntity",
                "/Skullmonkeys/FinnPlayerEntity", "/Skullmonkeys/RunnPlayerEntity",
                "/Skullmonkeys/SoarPlayerEntity", "/Skullmonkeys/PathEnemyEntity",
            };
            for (String s : spriteStructs) applyEnum(dtm, s, 0xe0, afR);

            ok = true;
        } finally {
            currentProgram.endTransaction(tx, ok);
        }
        println("CreateAuditEnums done (committed=" + ok + ")");
    }

    private void applyEnum(DataTypeManager dtm, String path, int off, DataType enumDt) {
        Structure s = (Structure) dtm.getDataType(path);
        if (s == null) { println("!! missing " + path); return; }
        DataTypeComponent c = null;
        for (DataTypeComponent cc : s.getDefinedComponents())
            if (cc.getOffset() == off) { c = cc; break; }
        if (c == null) { println("!! no component " + path + "+0x" + Integer.toHexString(off)); return; }
        String name = c.getFieldName();
        String comment = c.getComment();
        if (c.getLength() != enumDt.getLength()) {
            println("!! size mismatch " + path + "+0x" + Integer.toHexString(off)
                    + " field=" + c.getLength() + " enum=" + enumDt.getLength() + " (skipped)");
            return;
        }
        s.replaceAtOffset(off, enumDt, enumDt.getLength(), name, comment);
        println("APPLY " + enumDt.getName() + " -> " + path + "+0x" + Integer.toHexString(off)
                + " (" + name + ")");
    }
}
