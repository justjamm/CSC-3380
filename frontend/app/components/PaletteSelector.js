"use client";

import { useEffect, useState } from "react";

const PALETTES = [
  { id: "corpo-green", label: "CORPO", swatch: "#00ff66" },
  { id: "militech-red", label: "MILITECH", swatch: "#ff1a3c" },
  { id: "arasaka-amber", label: "ARASAKA", swatch: "#ffae1a" },
  { id: "neon-yellow", label: "CAUTION", swatch: "#fcee0a" },
  { id: "night-city", label: "NIGHT CITY", swatch: "#ff2bd6" },
];

const STORAGE_KEY = "hud.palette";
const DEFAULT_PALETTE = "militech-red";

export default function PaletteSelector() {
  const [active, setActive] = useState(() => {
    if (typeof window === "undefined") {
      return DEFAULT_PALETTE;
    }

    try {
      const stored = window.localStorage.getItem(STORAGE_KEY);
      if (PALETTES.some((palette) => palette.id === stored)) {
        return stored;
      }
    } catch {
      // ignore storage errors
    }

    return DEFAULT_PALETTE;
  });

  useEffect(() => {
    document.documentElement.dataset.palette = active;
    try {
      window.localStorage.setItem(STORAGE_KEY, active);
    } catch {
      // ignore storage errors
    }
  }, [active]);

  return (
    <div
      className="starting-fade"
      aria-label="Palette selector"
    >
      <div className="rounded-lg border border-accent/35 bg-black/35 p-2.5 inset-shadow-sm inset-shadow-accent/10">
        <p className="font-display text-[10px] font-bold uppercase tracking-[0.3em] text-accent/70">
          {"HUD // PALETTE"}
        </p>
        <div className="mt-2 flex flex-wrap items-center gap-1.5">
          {PALETTES.map((palette) => {
            const isActive = palette.id === active;
            return (
              <button
                key={palette.id}
                type="button"
                onClick={() => setActive(palette.id)}
                className={`group relative h-5 w-5 rounded-full border transition-all duration-200 sm:h-6 sm:w-6 ${
                  isActive
                    ? "scale-110 border-white/80 shadow-[0_0_10px_var(--swatch)]"
                    : "border-white/20 hover:scale-105 hover:border-white/50"
                }`}
                style={{ background: palette.swatch, "--swatch": palette.swatch }}
                aria-label={`Activate ${palette.label} palette`}
                aria-pressed={isActive}
                title={palette.label}
              >
                {isActive && (
                  <span
                    className="pointer-events-none absolute -bottom-[3px] left-1/2 h-[3px] w-[3px] -translate-x-1/2 rounded-full bg-white"
                  />
                )}
              </button>
            );
          })}
        </div>
      </div>
    </div>
  );
}
