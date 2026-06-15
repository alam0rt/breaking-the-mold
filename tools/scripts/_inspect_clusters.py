"""Find suffix pairs whose calcHash XOR has popcount 2 (cluster 0x00000003)."""
import sys
sys.path.insert(0, 'tools/scripts')
from compound_hash_attack import calc_hash_and_shift, rotl

CANDIDATES = (
    list("0123456789ABCDEF") +
    ["01","02","03","04","05","06","07","08","09",
     "10","11","12","13","14","15","16","17","18","19",
     "20","21","22","23","24","25","26","27","28","29",
     "30","31","32"] +
    ["A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P",
     "Q","R","S","T","U","V","W","X","Y","Z"] +
    ["LEFT","RIGHT","UP","DOWN","FRONT","BACK","NORTH","SOUTH","EAST","WEST",
     "TOP","BOTTOM","SIDE","REAR","FACE","INNER","OUTER","ABOVE","BELOW",
     "STAND","WALK","RUN","JUMP","FALL","IDLE","DIE","ATTACK","ATK",
     "HURT","HIT","SHOOT","FIRE","SPIT","BOUNCE","SLAM","ROLL",
     "OPEN","CLOSE","START","END","INTRO","OUTRO",
     "L","R","U","D","H","V"]
)


def classify(V: int) -> int:
    return min(rotl(V, r) for r in range(32))


def search(class_target: int, max_popcount: int = 8) -> list[tuple[str, str, int, int, int]]:
    seen = set()
    matches = []
    for a in CANDIDATES:
        ha, sa = calc_hash_and_shift(a)
        for b in CANDIDATES:
            if a >= b:
                continue
            hb, sb = calc_hash_and_shift(b)
            V = ha ^ hb
            if V == 0:
                continue
            if bin(V).count("1") > max_popcount:
                continue
            cls = classify(V)
            if cls == class_target:
                if (a, b) in seen:
                    continue
                seen.add((a, b))
                matches.append((a, b, V, sa, sb))
    return matches


for cls_target in (0x00000003, 0x00000005, 0x00000009, 0x00000011, 0x00000021, 0x00000041, 0x00000081, 0x00000303, 0x00283063, 0x007850a5, 0x0040c685):
    print(f"=== class 0x{cls_target:08x} ===")
    ms = search(cls_target, max_popcount=10)
    for a, b, V, sa, sb in ms[:20]:
        print(f"  {a:8s}  ↔  {b:8s}  V=0x{V:08x}  shift_after={sa}")
    if not ms:
        print("  (no obvious 1-token suffix matches)")
