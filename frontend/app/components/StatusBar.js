"use client";

import { motion } from "framer-motion";

const STATUS_CONFIG = {
  connected: { color: "bg-accent", label: "CONNECTED", tone: "accent" },
  reconnecting: { color: "bg-warn", label: "RECONNECTING", tone: "warn" },
  error: { color: "bg-danger", label: "ERROR", tone: "danger" },
  offline: { color: "bg-white/30", label: "OFFLINE", tone: "muted" },
};

export default function StatusBar({ wsStatus, lastRealtimeAt, error }) {
  const { color, label } = STATUS_CONFIG[wsStatus] || STATUS_CONFIG.offline;
  const isConnected = wsStatus === "connected";

  return (
    <div className="fixed top-[calc(env(safe-area-inset-top)+0.75rem)] right-3 left-3 z-20 rounded-lg border border-accent/40 bg-hud-surface p-2.5 backdrop-blur-sm inset-shadow-sm inset-shadow-accent/15 sm:top-auto sm:right-auto sm:bottom-[calc(5vh+0.75rem)] sm:left-[calc(5vw+0.75rem)] sm:max-w-xs sm:p-3">
      <div className="flex items-center gap-2 font-mono text-[11px] text-accent sm:text-xs">
        {isConnected ? (
          <motion.span
            animate={{ opacity: [1, 0.4, 1], boxShadow: ["0 0 4px var(--color-accent)", "0 0 12px var(--color-accent)", "0 0 4px var(--color-accent)"] }}
            transition={{ duration: 2, repeat: Infinity }}
            className={`inline-block h-2 w-2 rounded-full ${color}`}
          />
        ) : (
          <span className={`inline-block h-2 w-2 rounded-full ${color}`} />
        )}
        <span className="flicker font-display font-bold tracking-[0.25em] text-glow">
          {label}
        </span>
      </div>

      <p className="mt-1 font-mono text-[11px] text-accent/60 sm:text-xs">
        Last update:{" "}
        {lastRealtimeAt
          ? new Date(lastRealtimeAt).toLocaleTimeString()
          : "waiting..."}
      </p>

      {error && (
        <p className="text-glow-danger mt-1 break-words font-mono text-[11px] uppercase tracking-wider text-danger sm:text-xs">
          ! {error}
        </p>
      )}
    </div>
  );
}
