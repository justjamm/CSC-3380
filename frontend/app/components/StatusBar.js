"use client";

import { motion } from "framer-motion";

const STATUS_CONFIG = {
  connected: { color: "bg-green-500", label: "CONNECTED" },
  reconnecting: { color: "bg-yellow-500", label: "RECONNECTING" },
  error: { color: "bg-red-500", label: "ERROR" },
  offline: { color: "bg-gray-500", label: "OFFLINE" },
};

export default function StatusBar({ wsStatus, lastRealtimeAt, error }) {
  const { color, label } = STATUS_CONFIG[wsStatus] || STATUS_CONFIG.offline;
  const isConnected = wsStatus === "connected";

  return (
    <div className="fixed top-[calc(env(safe-area-inset-top)+0.75rem)] right-3 left-3 z-20 rounded-lg bg-black/60 p-2.5 backdrop-blur-sm sm:top-auto sm:right-auto sm:bottom-[0.5vh] sm:left-[5vw] sm:max-w-xs sm:p-3">
      <div className="flex items-center gap-2 font-mono text-[11px] text-green-400 sm:text-xs">
        {isConnected ? (
          <motion.span
            animate={{ opacity: [1, 0.4, 1] }}
            transition={{ duration: 2, repeat: Infinity }}
            className={`inline-block h-2 w-2 rounded-full ${color}`}
          />
        ) : (
          <span className={`inline-block h-2 w-2 rounded-full ${color}`} />
        )}
        <span className="text-glow-green">{label}</span>
      </div>

      <p className="mt-1 font-mono text-[11px] text-green-400/70 sm:text-xs">
        Last update:{" "}
        {lastRealtimeAt
          ? new Date(lastRealtimeAt).toLocaleTimeString()
          : "waiting..."}
      </p>

      {error && (
        <p className="mt-1 break-words font-mono text-[11px] text-red-400 sm:text-xs">{error}</p>
      )}
    </div>
  );
}
