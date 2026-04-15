"use client";

import { useState } from "react";
import { AnimatePresence, motion } from "framer-motion";

export default function SettingsPanel() {
  const [isSettingsOpen, setIsSettingsOpen] = useState(false);
  const [autoSwitchEnabled, setAutoSwitchEnabled] = useState(false);

  const handleToggleAutoSwitch = () => {
    setAutoSwitchEnabled((prev) => !prev);
  };

  return (
    <>
      {/* Settings Button */}
      <button
        type="button"
        onClick={() => setIsSettingsOpen((current) => !current)}
        className="fixed right-4 bottom-[calc(env(safe-area-inset-bottom)+8.25rem)] z-30 flex h-12 w-12 items-center justify-center rounded-lg border border-green-700/50 bg-black/70 backdrop-blur-sm transition-colors hover:bg-green-900/40 sm:right-[calc(5vw+0.75rem)] sm:bottom-[calc(5vh+8rem)]"
        aria-label={isSettingsOpen ? "Close settings" : "Open settings"}
      >
        <motion.div
          animate={{ rotate: isSettingsOpen ? 90 : 0 }}
          transition={{ type: "spring", stiffness: 300, damping: 20 }}
        >
          <svg width="20" height="20" viewBox="0 0 20 20" fill="none" xmlns="http://www.w3.org/2000/svg">
            <circle cx="10" cy="10" r="2" fill="#00ff41" />
            <path
              d="M10 3V1M10 19V17M17 10H19M1 10H3M15.657 15.657L17.071 17.071M2.929 2.929L4.343 4.343M15.657 4.343L17.071 2.929M4.343 15.657L2.929 17.071"
              stroke="#00ff41"
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
            className="fixed right-3 bottom-[calc(env(safe-area-inset-bottom)+12rem)] left-3 z-30 overflow-hidden rounded-xl border border-green-800/40 bg-gray-950/90 backdrop-blur-md sm:right-[5vw] sm:bottom-[calc(5vh+12rem)] sm:left-auto sm:w-80 sm:max-w-[38vw]"
          >
            <div className="border-b border-green-800/40 px-3 py-2">
              <p className="text-glow-green text-center font-mono text-[11px] uppercase tracking-[0.2em] text-green-400 sm:text-xs sm:tracking-[0.3em]">
                Settings
              </p>
            </div>

            <div className="p-4 space-y-4">
              {/* Auto Switch Toggle */}
              <div className="flex items-center justify-between">
                <label className="font-mono text-[11px] text-green-300 sm:text-xs cursor-pointer">
                  Auto Switch on Motion
                </label>
                <button
                  type="button"
                  onClick={handleToggleAutoSwitch}
                  className={`relative inline-flex h-6 w-11 items-center rounded-full transition-colors ${
                    autoSwitchEnabled ? "bg-green-600" : "bg-gray-700"
                  }`}
                  role="switch"
                  aria-checked={autoSwitchEnabled}
                >
                  <motion.span
                    layout
                    transition={{ type: "spring", stiffness: 500, damping: 30 }}
                    className={`inline-block h-5 w-5 transform rounded-full bg-white transition-transform ${
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
