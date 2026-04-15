"use client";

import PaletteSelector from "./PaletteSelector";

export default function ThemeShell({ children }) {
  return (
    <>
      <div className="vignette-overlay" aria-hidden />
      <div className="noise-overlay" aria-hidden />
      <PaletteSelector />
      {children}
    </>
  );
}
