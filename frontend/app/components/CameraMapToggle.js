"use client";

import { motion } from "framer-motion";

export default function CameraMapToggle({ isOpen, onToggle }) {
  return (
    <button
      type="button"
      onClick={onToggle}
      className="fixed right-4 bottom-[calc(env(safe-area-inset-bottom)+1rem)] z-30 flex h-12 w-12 items-center justify-center rounded-lg border border-green-700/50 bg-black/70 backdrop-blur-sm transition-colors hover:bg-green-900/40 sm:right-[calc(5vw+0.75rem)] sm:bottom-[calc(5vh+0.75rem)]"
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
          {/* 2x2 grid icon */}
          <rect x="1" y="1" width="7" height="7" rx="1" stroke="#00ff41" strokeWidth="1.5" />
          <rect x="12" y="1" width="7" height="7" rx="1" stroke="#00ff41" strokeWidth="1.5" />
          <rect x="1" y="12" width="7" height="7" rx="1" stroke="#00ff41" strokeWidth="1.5" />
          <rect x="12" y="12" width="7" height="7" rx="1" stroke="#00ff41" strokeWidth="1.5" />
        </svg>
      </motion.div>
    </button>
  );
}
