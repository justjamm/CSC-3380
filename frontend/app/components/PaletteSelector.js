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

export default function PaletteSelector() {
  const [active, setActive] = useState("militech-red");

  useEffect(() => {
    const stored = window.localStorage.getItem(STORAGE_KEY);
    const initial = PALETTES.some((p) => p.id === stored) ? stored : "militech-red";
    setActive(initial);
    document.documentElement.dataset.palette = initial;
  }, []);

  const selectPalette = (id) => {
    setActive(id);
    document.documentElement.dataset.palette = id;
    try {
      window.localStorage.setItem(STORAGE_KEY, id);
    } catch {
      // ignore storage errors
    }
  };

  return (
    <div
      className="starting-fade pointer-events-none fixed top-0 left-1/2 -translate-x-1/2 z-[999] flex w-max max-w-[96vw] justify-center px-2 pt-[calc(env(safe-area-inset-top)+0.5rem)]"
      aria-label="Palette selector"
    >
      <div className="hud-surface pointer-events-auto flex items-center gap-2 rounded-full border border-accent/50 px-3 py-1.5 shadow-[0_0_20px_color-mix(in_srgb,var(--color-accent)_20%,transparent)] inset-shadow-sm inset-shadow-accent/20">
        <span className="hidden font-display text-[9px] font-bold uppercase tracking-[0.35em] text-accent/70 sm:inline">
          SYS // PALETTE
        </span>
        <span className="font-display text-[9px] font-bold uppercase tracking-[0.3em] text-accent/70 sm:hidden">
          SYS
        </span>
        <div className="flex items-center gap-1.5">
          {PALETTES.map((palette) => {
            const isActive = palette.id === active;
            return (
              <button
                key={palette.id}
                type="button"
                onClick={() => selectPalette(palette.id)}
                className={`group relative h-5 w-5 rounded-full border transition-all duration-200 ${
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
