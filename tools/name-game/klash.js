// Skullmonkeys / ToolX asset-id hash — JS port of tools/klash.c
// asset_id(name) = 0x28C0E011 ^ rotl(calcHash(name), 27)
"use strict";

function rotl(v, r){ r &= 31; return ((v << r) | (v >>> ((32 - r) & 31))) >>> 0; }

function calcHash(s){
  let h = 0 >>> 0, sh = 0;
  for (let k = 0; k < s.length; k++){
    const oc = s.charCodeAt(k);          // original char (for digit test)
    let c = oc;
    if (c >= 97 && c <= 122) c -= 32;     // lowercase -> uppercase
    else if (c >= 48 && c <= 57) c += 22; // digits remap after letters
    const isAlpha = (c >= 65 && c <= 90);
    const isDigit = (oc >= 48 && oc <= 57);
    if (isAlpha || isDigit){
      sh = (sh + (c - 64)) & 31;
      h = (h ^ (1 << sh)) >>> 0;
    }
  }
  return h >>> 0;
}

function assetId(name){ return (0x28C0E011 ^ rotl(calcHash(name), 27)) >>> 0; }
function assetIdHex(name){ return "0x" + assetId(name).toString(16).padStart(8, "0"); }

function popcount32(v){ v = v >>> 0; let c = 0; while (v){ v &= v - 1; c++; } return c; }

// Honest lower bound on a name's length, derived from the id alone:
// each char sets at most one bit, and the finalizer (rotate + XOR const)
// preserves popcount, so popcount(id ^ SEED) <= number of name characters.
function minNameLen(id){ return popcount32((id >>> 0) ^ 0x28C0E011); }

// number of characters the hash actually counts (alphanumerics only)
function effectiveLen(s){ let n = 0; for (let i=0;i<s.length;i++){ const c=s.charCodeAt(i);
  if ((c>=48&&c<=57)||(c>=65&&c<=90)||(c>=97&&c<=122)) n++; } return n; }

// quick self-check (logs only on mismatch)
(function(){
  const t = { "NO":"0x29c0e211", "YES":"0x2ad0f011", "QUIT":"0x68c0f413",
              "CONTINUE":"0x69c04050", "QUIT GAME":"0x69c8f473", "PAUSED":"0x0ad0f813" };
  for (const [n,h] of Object.entries(t))
    if (assetIdHex(n) !== h) console.error("klash.js self-check FAIL", n, assetIdHex(n), "!=", h);
})();
