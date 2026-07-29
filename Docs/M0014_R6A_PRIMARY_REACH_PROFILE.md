# M0014-R6A — Primary Reach Landscape Profile Audit

**Milestone:** M0014-R6A · **Baseline:** `c82d276` · **Date:** 2026-07-29 · **Engine:** UE 5.8

Read-only measurement of the shipped Landscape against the authored water surface at every approved
Primary Reach station. Produced by `APrimaryReachHydrology::AuditLandscapeProfile()`, which writes no
property and stores no result. **No asset was modified to produce this report.**

## Method

At each of the 136 approved centreline stations the audit takes the spline point in world space,
traces straight down for WorldStatic collision via `FTerrainSurfaceQuery::TraceGroundZ`, and compares
the hit Z against that station's authored `WaterSurfaceElevation`.

`clearance = water_z - ground_z`. Positive means ground lies **below** the water surface (submerged);
negative means terrain stands **above** it. Classification uses a minimal floating-point epsilon of
±0.1 cm, not a correction tolerance: this audit measures the existing physical relationship between
ground and water. It does not decide how much deviation should be treated as acceptable — incision
depth and correction tolerance are R6B decisions, made later, against this data.

Trace misses are reported as MISS and left unmeasured. No missing value was substituted.

## Result summary

| Metric | Value |
|---|---|
| Stations evaluated | **136 / 136** |
| Trace failures | **0** |
| AT_SURFACE (within ±0.1 cm) | **74** |
| ABOVE water | **62** |
| BELOW water | **0** |
| Audit runs compared | **4, all byte-identical** |
| Authored water surface | `-87384.8` cm, constant across the whole reach |
| Ground Z range | `-87384.8` to `-87041.0` cm |
| Clearance range | `-343.8` to `0.0` cm |

## The decisive finding

**No sampled centreline ground lies below the authored water surface.** Maximum clearance is exactly
`0.0` cm, and the minimum ground Z equals the water Z to the decimal. Measured terrain is at the
authored water surface or above it, never beneath it.

This measurement establishes the existing physical relationship only. It does not itself define what
correction, if any, R6B should apply — incision depth and correction tolerance are R6B decisions, to
be made against this data.

## Measured terrain-relation zones

| State | First idx | Last idx | Start chainage (cm) | End chainage (cm) | Stations |
|---|---|---|---|---|---|
| ABOVE | 0 | 26 | 0.0 | 36599.9 | 27 |
| AT_SURFACE | 27 | 36 | 38520.3 | 49731.1 | 10 |
| ABOVE | 37 | 60 | 50922.0 | 81154.3 | 24 |
| AT_SURFACE | 61 | 124 | 82345.3 | 159744.9 | 64 |
| ABOVE | 125 | 135 | 161665.2 | 188650.5 | 11 |

Five contiguous zones alternate ABOVE / AT_SURFACE / ABOVE / AT_SURFACE / ABOVE along the reach:
idx 0–26 (ABOVE), idx 27–36 (AT_SURFACE), idx 37–60 (ABOVE), idx 61–124 (AT_SURFACE), idx 125–135
(ABOVE).

## Full station measurements

| idx | chainage (cm) | ground Z (cm) | water Z (cm) | clearance (cm) | trace | state |
|---|---|---|---|---|---|---|
| 0 | 0.0 | -87341.9 | -87384.8 | -42.9 | HIT | ABOVE |
| 1 | 1065.1 | -87339.3 | -87384.8 | -45.5 | HIT | ABOVE |
| 2 | 2130.3 | -87339.3 | -87384.8 | -45.5 | HIT | ABOVE |
| 3 | 3321.2 | -87339.3 | -87384.8 | -45.5 | HIT | ABOVE |
| 4 | 4386.5 | -87336.5 | -87384.8 | -48.3 | HIT | ABOVE |
| 5 | 5451.5 | -87336.5 | -87384.8 | -48.3 | HIT | ABOVE |
| 6 | 6642.5 | -87336.5 | -87384.8 | -48.3 | HIT | ABOVE |
| 7 | 7707.7 | -87333.8 | -87384.8 | -51.0 | HIT | ABOVE |
| 8 | 9214.0 | -87336.5 | -87384.8 | -48.3 | HIT | ABOVE |
| 9 | 10720.4 | -87336.5 | -87384.8 | -48.3 | HIT | ABOVE |
| 10 | 11911.3 | -87336.5 | -87384.8 | -48.3 | HIT | ABOVE |
| 11 | 13417.7 | -87336.5 | -87384.8 | -48.3 | HIT | ABOVE |
| 12 | 14608.5 | -87333.9 | -87384.8 | -50.9 | HIT | ABOVE |
| 13 | 16114.9 | -87336.5 | -87384.8 | -48.3 | HIT | ABOVE |
| 14 | 17621.4 | -87336.5 | -87384.8 | -48.3 | HIT | ABOVE |
| 15 | 19127.8 | -87336.5 | -87384.8 | -48.3 | HIT | ABOVE |
| 16 | 20634.1 | -87336.5 | -87384.8 | -48.3 | HIT | ABOVE |
| 17 | 22554.4 | -87339.2 | -87384.8 | -45.6 | HIT | ABOVE |
| 18 | 24060.8 | -87341.9 | -87384.8 | -42.9 | HIT | ABOVE |
| 19 | 25567.1 | -87341.9 | -87384.8 | -42.9 | HIT | ABOVE |
| 20 | 26632.3 | -87339.2 | -87384.8 | -45.6 | HIT | ABOVE |
| 21 | 28138.7 | -87341.9 | -87384.8 | -42.9 | HIT | ABOVE |
| 22 | 29645.1 | -87347.3 | -87384.8 | -37.5 | HIT | ABOVE |
| 23 | 30835.9 | -87352.6 | -87384.8 | -32.2 | HIT | ABOVE |
| 24 | 32026.9 | -87360.6 | -87384.8 | -24.2 | HIT | ABOVE |
| 25 | 34894.9 | -87376.7 | -87384.8 | -8.1 | HIT | ABOVE |
| 26 | 36599.9 | -87382.1 | -87384.8 | -2.7 | HIT | ABOVE |
| 27 | 38520.3 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 28 | 39851.8 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 29 | 41772.0 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 30 | 42962.8 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 31 | 44153.8 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 32 | 45344.7 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 33 | 46409.9 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 34 | 47475.0 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 35 | 48540.2 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 36 | 49731.1 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 37 | 50922.0 | -87382.1 | -87384.8 | -2.7 | HIT | ABOVE |
| 38 | 51987.2 | -87376.7 | -87384.8 | -8.1 | HIT | ABOVE |
| 39 | 53178.1 | -87366.0 | -87384.8 | -18.8 | HIT | ABOVE |
| 40 | 54684.5 | -87347.3 | -87384.8 | -37.5 | HIT | ABOVE |
| 41 | 55875.3 | -87328.6 | -87384.8 | -56.2 | HIT | ABOVE |
| 42 | 57381.7 | -87304.2 | -87384.8 | -80.6 | HIT | ABOVE |
| 43 | 58888.2 | -87282.9 | -87384.8 | -101.9 | HIT | ABOVE |
| 44 | 60079.0 | -87272.0 | -87384.8 | -112.8 | HIT | ABOVE |
| 45 | 61269.9 | -87266.7 | -87384.8 | -118.1 | HIT | ABOVE |
| 46 | 62776.3 | -87261.2 | -87384.8 | -123.6 | HIT | ABOVE |
| 47 | 63841.5 | -87266.7 | -87384.8 | -118.1 | HIT | ABOVE |
| 48 | 65032.3 | -87266.7 | -87384.8 | -118.1 | HIT | ABOVE |
| 49 | 66538.7 | -87266.7 | -87384.8 | -118.1 | HIT | ABOVE |
| 50 | 67603.9 | -87272.0 | -87384.8 | -112.8 | HIT | ABOVE |
| 51 | 69110.3 | -87269.4 | -87384.8 | -115.4 | HIT | ABOVE |
| 52 | 70175.5 | -87277.4 | -87384.8 | -107.4 | HIT | ABOVE |
| 53 | 71681.8 | -87280.2 | -87384.8 | -104.6 | HIT | ABOVE |
| 54 | 73188.3 | -87288.1 | -87384.8 | -96.7 | HIT | ABOVE |
| 55 | 74379.1 | -87304.2 | -87384.8 | -80.6 | HIT | ABOVE |
| 56 | 75885.5 | -87325.7 | -87384.8 | -59.1 | HIT | ABOVE |
| 57 | 76950.7 | -87341.9 | -87384.8 | -42.9 | HIT | ABOVE |
| 58 | 78457.1 | -87363.4 | -87384.8 | -21.4 | HIT | ABOVE |
| 59 | 79647.9 | -87374.1 | -87384.8 | -10.7 | HIT | ABOVE |
| 60 | 81154.3 | -87382.1 | -87384.8 | -2.7 | HIT | ABOVE |
| 61 | 82345.3 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 62 | 83851.6 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 63 | 85358.0 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 64 | 86423.2 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 65 | 87614.1 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 66 | 89120.4 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 67 | 90185.6 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 68 | 91376.6 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 69 | 92441.7 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 70 | 93632.6 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 71 | 95138.9 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 72 | 96204.1 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 73 | 97395.0 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 74 | 98901.4 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 75 | 100092.3 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 76 | 101283.2 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 77 | 102474.1 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 78 | 103980.5 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 79 | 105171.4 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 80 | 106362.3 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 81 | 107427.4 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 82 | 108618.4 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 83 | 109809.3 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 84 | 111000.1 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 85 | 112065.3 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 86 | 113571.7 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 87 | 114636.8 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 88 | 115702.0 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 89 | 117208.4 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 90 | 118273.6 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 91 | 119464.5 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 92 | 120655.4 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 93 | 121846.3 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 94 | 122911.4 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 95 | 124102.3 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 96 | 125167.5 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 97 | 126358.4 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 98 | 127423.5 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 99 | 128488.7 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 100 | 129553.9 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 101 | 130619.0 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 102 | 131809.9 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 103 | 132875.1 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 104 | 134066.0 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 105 | 135256.9 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 106 | 136322.1 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 107 | 137387.3 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 108 | 138578.2 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 109 | 139643.3 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 110 | 140708.5 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 111 | 141773.7 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 112 | 142838.8 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 113 | 144345.2 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 114 | 145410.4 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 115 | 146475.6 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 116 | 147666.5 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 117 | 148857.4 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 118 | 150048.3 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 119 | 151239.1 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 120 | 152430.0 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 121 | 153936.5 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 122 | 155442.9 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 123 | 157363.1 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 124 | 159744.9 | -87384.8 | -87384.8 | 0.0 | HIT | AT_SURFACE |
| 125 | 161665.2 | -87376.7 | -87384.8 | -8.1 | HIT | ABOVE |
| 126 | 167619.7 | -87339.3 | -87384.8 | -45.5 | HIT | ABOVE |
| 127 | 169126.0 | -87336.5 | -87384.8 | -48.3 | HIT | ABOVE |
| 128 | 171046.4 | -87344.5 | -87384.8 | -40.3 | HIT | ABOVE |
| 129 | 172237.3 | -87358.0 | -87384.8 | -26.8 | HIT | ABOVE |
| 130 | 174157.5 | -87379.5 | -87384.8 | -5.3 | HIT | ABOVE |
| 131 | 176077.8 | -87379.5 | -87384.8 | -5.3 | HIT | ABOVE |
| 132 | 179446.1 | -87301.5 | -87384.8 | -83.3 | HIT | ABOVE |
| 133 | 183068.1 | -87156.5 | -87384.8 | -228.3 | HIT | ABOVE |
| 134 | 186945.4 | -87046.4 | -87384.8 | -338.4 | HIT | ABOVE |
| 135 | 188650.5 | -87041.0 | -87384.8 | -343.8 | HIT | ABOVE |

## Verification

- Closed-editor `DestructionEditor Win64 Development` build: **Succeeded**, both before and after the
  classification correction (epsilon changed from an invented ±50 cm tolerance to a minimal ±0.1 cm
  floating-point epsilon; ground and water Z values are unchanged).
- Four consecutive live audit runs (prior to the classification correction) produced identical raw
  ground/water measurements, compared line by line.
- The ±0.1 cm classification was recomputed against those unchanged raw rows and independently
  verified to produce exactly 74 AT_SURFACE, 62 ABOVE, 0 BELOW, with ABOVE runs at idx 0–26, 37–60,
  125–135 and AT_SURFACE runs at idx 27–36, 61–124 — matching the Technical Director's expected result.
- `git status` shows no modified `Content/`, level, Landscape, or ExternalActor file.

R6B remains unimplemented.
