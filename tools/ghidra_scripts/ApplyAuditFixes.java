// Apply approved naming/congruence fixes to the Skullmonkeys program.
// Run headless (analyzeHeadless saves automatically):
//   ghidra-analyzeHeadless /mnt/share/sam/ghidra skullmonkeys \
//     -process SLES_010.90 -noanalysis \
//     -scriptPath <repo>/tools/ghidra_scripts -postScript ApplyAuditFixes.java
//@category Skullmonkeys
import ghidra.app.script.GhidraScript;
import ghidra.program.model.data.DataType;
import ghidra.program.model.data.DataTypeComponent;
import ghidra.program.model.data.DataTypeManager;
import ghidra.program.model.data.ShortDataType;
import ghidra.program.model.data.Structure;

public class ApplyAuditFixes extends GhidraScript {

    @Override
    public void run() throws Exception {
        DataTypeManager dtm = currentProgram.getDataTypeManager();
        int tx = currentProgram.startTransaction("audit: struct naming/congruence fixes");
        boolean ok = false;
        try {
            // #1 retype RunnPlayerEntity +0x6e -> short velocityY
            retypeShort(dtm, "/Skullmonkeys/RunnPlayerEntity", 0x6e, "velocityY", "Y velocity");

            // #2 unify divergent shared sprite-block field names
            rename(dtm, "/Skullmonkeys/SpriteEntity", 0x8a, "texPageByte");
            rename(dtm, "/Skullmonkeys/SpriteEntity", 0x98, "nextStateMarker");
            rename(dtm, "/Skullmonkeys/SpriteEntity", 0x9c, "nextStateCallback");
            rename(dtm, "/Skullmonkeys/PlayerEntity", 0xa0, "activeStateMarker");
            rename(dtm, "/Skullmonkeys/PlayerEntity", 0xa8, "exitCallbackMarker");
            rename(dtm, "/Skullmonkeys/PlayerEntity", 0xe2, "sequenceStep");
            rename(dtm, "/Skullmonkeys/PlayerEntity", 0xe4, "sequenceLength");

            // #6 delete tombstoned obsolete type
            DataType obsolete = dtm.getDataType("/game/ObsoleteSpriteTypeCallbackEntry");
            if (obsolete != null) {
                if (dtm.remove(obsolete, monitor)) {
                    println("DELETE /game/ObsoleteSpriteTypeCallbackEntry");
                } else {
                    println("!! could not delete /game/ObsoleteSpriteTypeCallbackEntry");
                }
            } else {
                println("skip delete: /game/ObsoleteSpriteTypeCallbackEntry not found");
            }
            ok = true;
        } finally {
            currentProgram.endTransaction(tx, ok);
        }
        println("ApplyAuditFixes done (committed=" + ok + ")");
    }

    private void rename(DataTypeManager dtm, String path, int off, String newName) throws Exception {
        Structure s = (Structure) dtm.getDataType(path);
        if (s == null) { println("!! missing " + path); return; }
        for (DataTypeComponent c : s.getDefinedComponents()) {
            if (c.getOffset() == off) {
                String old = c.getFieldName();
                if (newName.equals(old)) { println("skip " + path + "+0x" + Integer.toHexString(off) + ": already " + newName); return; }
                c.setFieldName(newName);
                println("RENAME " + path + "+0x" + Integer.toHexString(off) + ": " + old + " -> " + newName);
                return;
            }
        }
        println("!! no component " + path + "+0x" + Integer.toHexString(off));
    }

    private void retypeShort(DataTypeManager dtm, String path, int off, String newName, String comment) throws Exception {
        Structure s = (Structure) dtm.getDataType(path);
        if (s == null) { println("!! missing " + path); return; }
        DataTypeComponent before = s.getComponentContaining(off);
        String old = before == null ? "(none)" : before.getDataType().getName() + " " + before.getFieldName();
        s.replaceAtOffset(off, ShortDataType.dataType, 2, newName, comment);
        println("RETYPE " + path + "+0x" + Integer.toHexString(off) + ": " + old + " -> short " + newName);
    }
}
