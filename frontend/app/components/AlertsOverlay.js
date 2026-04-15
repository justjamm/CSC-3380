"use client";

import { motion, AnimatePresence } from "framer-motion";

const SEVERITY_COLORS = {
  critical: "border-red-500",
  high: "border-red-500",
  medium: "border-yellow-400",
  warning: "border-yellow-400",
  low: "border-green-400",
  info: "border-green-400",
};

function getSeverityBorder(severity) {
  return SEVERITY_COLORS[severity?.toLowerCase()] || "border-gray-500";
}

export default function AlertsOverlay({ alerts }) {
  return (
    <div className="hud-scrollbar fixed top-[calc(env(safe-area-inset-top)+5.75rem)] right-3 left-3 z-20 max-h-[32vh] overflow-y-auto rounded-lg bg-black/60 p-2.5 backdrop-blur-sm sm:top-[5vh] sm:right-[5vw] sm:left-auto sm:max-h-[60vh] sm:w-72 sm:p-3">
      <h2 className="text-glow-green mb-2 font-mono text-xs uppercase tracking-widest text-green-400 sm:text-sm">
        Alerts
      </h2>

      {alerts.length === 0 ? (
        <p className="font-mono text-[11px] text-gray-500 sm:text-xs">NO ACTIVE ALERTS</p>
      ) : (
        <AnimatePresence initial={false}>
          {alerts.map((alert) => (
            <motion.div
              key={alert.id}
              initial={{ opacity: 0, x: 20 }}
              animate={{ opacity: 1, x: 0 }}
              exit={{ opacity: 0, x: 20 }}
              transition={{ duration: 0.2 }}
              className={`mb-2 border-l-2 ${getSeverityBorder(alert.severity)} pl-2`}
            >
              <p className="font-mono text-[11px] font-medium text-green-300 sm:text-xs">
                {alert.id}
              </p>
              <p className="font-mono text-[11px] uppercase text-yellow-300 sm:text-xs">
                {alert.severity || "info"}
              </p>
              <p className="font-mono text-[11px] text-gray-400 sm:text-xs">
                {alert.message}
              </p>
            </motion.div>
          ))}
        </AnimatePresence>
      )}
    </div>
  );
}
