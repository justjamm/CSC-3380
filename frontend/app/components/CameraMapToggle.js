"use client";

import { motion } from "framer-motion";

export default function CameraMapToggle({ isOpen, onToggle }) {
  return (
    <button
      type="button"
      onClick={onToggle}
      className="fixed right-4 bottom-[calc(env(safe-area-inset-bottom)+1rem)] z-30 flex h-12 w-12 items-center justify-center rounded-lg border border-accent/50 bg-hud-surface text-accent backdrop-blur-sm transition-colors hover:bg-accent/15 hover:shadow-[0_0_14px_color-mix(in_srgb,var(--color-accent)_40%,transparent)] inset-shadow-sm inset-shadow-accent/20 sm:right-[calc(5vw+0.75rem)] sm:bottom-[calc(5vh+0.75rem)]"
      aria-label={isOpen ? "Close camera map" : "Open camera map"}
    >
      <motion.div
        animate={{ rotate: isOpen ? 45 : 0 }}
        transition={{ type: "spring", stiffness: 300, damping: 20 }}
      >
        <svg
          width="20"
          height="20"
          viewBox="0 0 20 20"
          fill="none"
          xmlns="http://www.w3.org/2000/svg"
        >
          <rect x="1" y="1" width="7" height="7" rx="1" stroke="currentColor" strokeWidth="1.5" />
          <rect x="12" y="1" width="7" height="7" rx="1" stroke="currentColor" strokeWidth="1.5" />
          <rect x="1" y="12" width="7" height="7" rx="1" stroke="currentColor" strokeWidth="1.5" />
          <rect x="12" y="12" width="7" height="7" rx="1" stroke="currentColor" strokeWidth="1.5" />
        </svg>
      </motion.div>
    </button>
  );
}
