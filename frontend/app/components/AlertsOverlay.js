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
    <div className="hud-scrollbar fixed top-[5vh] right-[5vw] z-20 max-h-[60vh] w-72 overflow-y-auto rounded-lg bg-black/60 p-3 backdrop-blur-sm">
      <h2 className="text-glow-green mb-2 font-mono text-sm uppercase tracking-widest text-green-400">
        Alerts
      </h2>

      {alerts.length === 0 ? (
        <p className="font-mono text-xs text-gray-500">NO ACTIVE ALERTS</p>
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
              <p className="font-mono text-xs font-medium text-green-300">
                {alert.id}
              </p>
              <p className="font-mono text-xs uppercase text-yellow-300">
                {alert.severity || "info"}
              </p>
              <p className="font-mono text-xs text-gray-400">
                {alert.message}
              </p>
            </motion.div>
          ))}
        </AnimatePresence>
      )}
    </div>
  );
}
