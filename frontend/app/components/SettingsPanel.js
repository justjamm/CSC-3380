"use client";

import { useState } from "react";
import { AnimatePresence, motion } from "framer-motion";
import PaletteSelector from "./PaletteSelector";

export default function SettingsPanel({ autoSwitchEnabled = false, onAutoSwitchToggle }) {
  const [isSettingsOpen, setIsSettingsOpen] = useState(false);

  const handleToggleAutoSwitch = () => {
    if (typeof onAutoSwitchToggle === "function") {
      onAutoSwitchToggle(!autoSwitchEnabled);
    }
  };

  return (
    <>
      {/* Settings Button */}
      <button
        type="button"
        onClick={() => setIsSettingsOpen((current) => !current)}
        className="fixed right-4 bottom-[calc(env(safe-area-inset-bottom)+8.25rem)] z-30 flex h-12 w-12 items-center justify-center rounded-lg border border-accent/50 bg-hud-surface text-accent backdrop-blur-sm transition-colors hover:bg-accent/15 hover:shadow-[0_0_14px_color-mix(in_srgb,var(--color-accent)_40%,transparent)] inset-shadow-sm inset-shadow-accent/20 sm:right-[calc(5vw+0.75rem)] sm:bottom-[calc(5vh+8rem)]"
        aria-label={isSettingsOpen ? "Close settings" : "Open settings"}
      >
        <motion.div
          animate={{ rotate: isSettingsOpen ? 90 : 0 }}
          transition={{ type: "spring", stiffness: 300, damping: 20 }}
        >
          <svg width="20" height="20" viewBox="0 0 20 20" fill="none" xmlns="http://www.w3.org/2000/svg">
            <circle cx="10" cy="10" r="2" fill="currentColor" />
            <path
              d="M10 3V1M10 19V17M17 10H19M1 10H3M15.657 15.657L17.071 17.071M2.929 2.929L4.343 4.343M15.657 4.343L17.071 2.929M4.343 15.657L2.929 17.071"
              stroke="currentColor"
              strokeWidth="1.5"
              strokeLinecap="round"
              strokeLinejoin="round"
            />
          </svg>
        </motion.div>
      </button>

      {/* Settings Panel */}
      <AnimatePresence>
        {isSettingsOpen && (
          <motion.section
            initial={{ opacity: 0, y: 40, scale: 0.95 }}
            animate={{ opacity: 1, y: 0, scale: 1 }}
            exit={{ opacity: 0, y: 40, scale: 0.95 }}
            transition={{ type: "spring", damping: 25, stiffness: 300 }}
            className="fixed right-3 bottom-[calc(env(safe-area-inset-bottom)+12rem)] left-3 z-30 overflow-hidden rounded-xl border border-accent/40 bg-hud-panel backdrop-blur-md inset-shadow-sm inset-shadow-accent/15 sm:right-[5vw] sm:bottom-[calc(5vh+12rem)] sm:left-auto sm:w-80 sm:max-w-[38vw]"
          >
            <div className="relative border-b border-accent/30 px-3 py-2">
              <p className="hud-title text-center text-[11px] sm:text-xs">
                {"// SETTINGS"}
              </p>
            </div>

            <div className="space-y-4 p-4">
              <PaletteSelector />

              {/* Auto Switch Toggle */}
              <div className="flex items-center justify-between">
                <label className="cursor-pointer font-mono text-[11px] text-foreground/80 sm:text-xs">
                  Auto Switch on Motion
                </label>
                <button
                  type="button"
                  onClick={handleToggleAutoSwitch}
                  className={`relative inline-flex h-6 w-11 items-center rounded-full border transition-all duration-200 ${
                    autoSwitchEnabled
                      ? "border-accent/70 bg-accent/80 shadow-[0_0_12px_color-mix(in_srgb,var(--color-accent)_45%,transparent)]"
                      : "border-accent/35 bg-black/45"
                  }`}
                  role="switch"
                  aria-checked={autoSwitchEnabled}
                >
                  <motion.span
                    layout
                    transition={{ type: "spring", stiffness: 500, damping: 30 }}
                    className={`inline-block h-5 w-5 transform rounded-full bg-foreground transition-transform ${
                      autoSwitchEnabled ? "translate-x-5" : "translate-x-0"
                    }`}
                  />
                </button>
              </div>

            </div>
          </motion.section>
        )}
      </AnimatePresence>
    </>
  );
}
