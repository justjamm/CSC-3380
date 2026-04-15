"use client";

import { useCallback, useEffect, useMemo, useRef, useState } from "react";
import { AnimatePresence, motion } from "framer-motion";

const MAX_TOASTS = 4;
const TOAST_LIFETIME_MS = 5000;
const MAX_SEEN_ALERT_KEYS = 800;

const SEVERITY_STYLES = {
  critical: {
    border: "border-red-500",
    text: "text-red-300",
    badgeBackground: "bg-red-950/40",
  },
  high: {
    border: "border-red-500",
    text: "text-red-300",
    badgeBackground: "bg-red-950/40",
  },
  medium: {
    border: "border-yellow-400",
    text: "text-yellow-300",
    badgeBackground: "bg-yellow-950/40",
  },
  warning: {
    border: "border-yellow-400",
    text: "text-yellow-300",
    badgeBackground: "bg-yellow-950/40",
  },
  low: {
    border: "border-green-400",
    text: "text-green-300",
    badgeBackground: "bg-green-950/40",
  },
  info: {
    border: "border-green-400",
    text: "text-green-300",
    badgeBackground: "bg-green-950/40",
  },
  default: {
    border: "border-gray-500",
    text: "text-gray-300",
    badgeBackground: "bg-gray-900/40",
  },
};

function getSeverityStyle(severity) {
  const normalized = typeof severity === "string" ? severity.toLowerCase() : "default";
  return SEVERITY_STYLES[normalized] || SEVERITY_STYLES.default;
}

function getAlertFingerprint(alert, fallbackIndex = 0) {
  const id = typeof alert?.id === "string" && alert.id ? alert.id : `alert-${fallbackIndex}`;
  const timestamp = Number.isFinite(alert?.timestampMs) ? String(alert.timestampMs) : "na";
  const state = typeof alert?.state === "string" ? alert.state : "unknown";
  const message = typeof alert?.message === "string" ? alert.message : "";
  return `${id}|${timestamp}|${state}|${message}`;
}

function sortAlertsNewestFirst(sourceAlerts) {
  const list = Array.isArray(sourceAlerts) ? sourceAlerts.filter(Boolean) : [];
  return [...list].sort((left, right) => {
    const leftTimestamp = Number.isFinite(left?.timestampMs) ? left.timestampMs : -1;
    const rightTimestamp = Number.isFinite(right?.timestampMs) ? right.timestampMs : -1;
    return rightTimestamp - leftTimestamp;
  });
}

function formatTimestamp(timestampMs) {
  if (!Number.isFinite(timestampMs)) {
    return "Time unavailable";
  }
  return new Date(timestampMs).toLocaleString();
}

export default function AlertsOverlay({ alerts }) {
  const [toasts, setToasts] = useState([]);
  const [isHistoryOpen, setIsHistoryOpen] = useState(false);

  const initializedRef = useRef(false);
  const seenKeysRef = useRef(new Set());
  const seenOrderRef = useRef([]);
  const timersRef = useRef(new Map());
  const toastSequenceRef = useRef(0);

  const sortedAlerts = useMemo(() => sortAlertsNewestFirst(alerts), [alerts]);

  const rememberSeenKey = useCallback((key) => {
    if (seenKeysRef.current.has(key)) {
      return;
    }
    seenKeysRef.current.add(key);
    seenOrderRef.current.push(key);

    if (seenOrderRef.current.length > MAX_SEEN_ALERT_KEYS) {
      const oldestKey = seenOrderRef.current.shift();
      if (oldestKey) {
        seenKeysRef.current.delete(oldestKey);
      }
    }
  }, []);

  const removeToast = useCallback((toastId) => {
    setToasts((current) => current.filter((toast) => toast.toastId !== toastId));

    const timeoutId = timersRef.current.get(toastId);
    if (timeoutId) {
      clearTimeout(timeoutId);
      timersRef.current.delete(toastId);
    }
  }, []);

  useEffect(() => {
    if (!initializedRef.current) {
      sortedAlerts.forEach((alert, index) => {
        rememberSeenKey(getAlertFingerprint(alert, index));
      });
      initializedRef.current = true;
      return;
    }

    if (sortedAlerts.length === 0) {
      return;
    }

    const incoming = [];
    for (let index = sortedAlerts.length - 1; index >= 0; index -= 1) {
      const alert = sortedAlerts[index];
      const fingerprint = getAlertFingerprint(alert, index);

      if (!seenKeysRef.current.has(fingerprint)) {
        rememberSeenKey(fingerprint);
        incoming.push({ alert, fingerprint });
      }
    }

    if (incoming.length === 0) {
      return;
    }

    setToasts((current) => {
      let nextToasts = current;

      incoming.forEach(({ alert, fingerprint }) => {
        toastSequenceRef.current += 1;
        nextToasts = [
          { toastId: `${toastSequenceRef.current}-${fingerprint}`, alert },
          ...nextToasts,
        ];
      });

      return nextToasts.slice(0, MAX_TOASTS);
    });
  }, [rememberSeenKey, sortedAlerts]);

  useEffect(() => {
    const activeToastIds = new Set(toasts.map((toast) => toast.toastId));

    toasts.forEach((toast) => {
      if (timersRef.current.has(toast.toastId)) {
        return;
      }

      const timeoutId = setTimeout(() => {
        removeToast(toast.toastId);
      }, TOAST_LIFETIME_MS);
      timersRef.current.set(toast.toastId, timeoutId);
    });

    for (const [toastId, timeoutId] of timersRef.current.entries()) {
      if (!activeToastIds.has(toastId)) {
        clearTimeout(timeoutId);
        timersRef.current.delete(toastId);
      }
    }
  }, [removeToast, toasts]);

  useEffect(
    () => () => {
      for (const timeoutId of timersRef.current.values()) {
        clearTimeout(timeoutId);
      }
      timersRef.current.clear();
    },
    [],
  );

  return (
    <>
      <div className="pointer-events-none fixed top-[calc(env(safe-area-inset-top)+5.75rem)] right-3 left-3 z-30 flex flex-col gap-2 sm:top-[5vh] sm:right-[5vw] sm:left-auto sm:w-80">
        <AnimatePresence initial={false}>
          {toasts.map(({ toastId, alert }) => {
            const style = getSeverityStyle(alert?.severity);
            return (
              <motion.div
                layout
                key={toastId}
                initial={{ opacity: 0, y: -18, scale: 0.96 }}
                animate={{ opacity: 1, y: 0, scale: 1 }}
                exit={{ opacity: 0, y: -14, scale: 0.96 }}
                transition={{ type: "spring", stiffness: 360, damping: 28 }}
                className={`pointer-events-auto rounded-lg border border-green-900/50 border-l-4 ${style.border} bg-black/75 p-2.5 backdrop-blur-sm`}
              >
                <p className="truncate font-mono text-[11px] font-medium text-green-300 sm:text-xs">
                  {alert?.id || "alert"}
                </p>
                <p
                  className={`mt-1 inline-block rounded px-1.5 py-0.5 font-mono text-[10px] uppercase sm:text-[11px] ${style.text} ${style.badgeBackground}`}
                >
                  {alert?.severity || "info"}
                </p>
                <p className="mt-1 line-clamp-2 font-mono text-[11px] text-gray-300 sm:text-xs">
                  {alert?.message || "No alert message provided"}
                </p>
              </motion.div>
            );
          })}
        </AnimatePresence>
      </div>

      <button
        type="button"
        onClick={() => setIsHistoryOpen((current) => !current)}
        className="fixed right-4 bottom-[calc(env(safe-area-inset-bottom)+4.5rem)] z-30 flex h-12 w-12 items-center justify-center rounded-lg border border-green-700/50 bg-black/70 backdrop-blur-sm transition-colors hover:bg-green-900/40 sm:right-[calc(5vw+0.75rem)] sm:bottom-[calc(5vh+4.25rem)]"
        aria-label={isHistoryOpen ? "Close alerts history" : "Open alerts history"}
      >
        <motion.div
          animate={{ rotate: isHistoryOpen ? 45 : 0 }}
          transition={{ type: "spring", stiffness: 300, damping: 20 }}
        >
          <svg width="20" height="20" viewBox="0 0 20 20" fill="none" xmlns="http://www.w3.org/2000/svg">
            <rect x="2" y="3" width="16" height="2" rx="1" fill="#00ff41" />
            <rect x="2" y="9" width="16" height="2" rx="1" fill="#00ff41" />
            <rect x="2" y="15" width="16" height="2" rx="1" fill="#00ff41" />
          </svg>
        </motion.div>
      </button>

      <AnimatePresence>
        {isHistoryOpen && (
          <motion.section
            initial={{ opacity: 0, y: 40, scale: 0.95 }}
            animate={{ opacity: 1, y: 0, scale: 1 }}
            exit={{ opacity: 0, y: 40, scale: 0.95 }}
            transition={{ type: "spring", damping: 25, stiffness: 300 }}
            className="fixed right-3 bottom-[calc(env(safe-area-inset-bottom)+8.25rem)] left-3 z-30 overflow-hidden rounded-xl border border-green-800/40 bg-gray-950/90 backdrop-blur-md sm:right-[5vw] sm:bottom-[calc(5vh+8rem)] sm:left-auto sm:w-[24rem] sm:max-w-[38vw]"
          >
            <div className="border-b border-green-800/40 px-3 py-2">
              <p className="text-glow-green text-center font-mono text-[11px] uppercase tracking-[0.2em] text-green-400 sm:text-xs sm:tracking-[0.3em]">
                Alert History
              </p>
            </div>

            <div className="hud-scrollbar max-h-[48vh] overflow-y-auto p-3">
              {sortedAlerts.length === 0 ? (
                <p className="font-mono text-[11px] text-gray-500 sm:text-xs">NO ALERTS RECORDED</p>
              ) : (
                sortedAlerts.map((alert, index) => {
                  const style = getSeverityStyle(alert?.severity);
                  const itemKey = getAlertFingerprint(alert, index);

                  return (
                    <div
                      key={itemKey}
                      className={`mb-2 rounded-md border border-green-900/40 border-l-4 ${style.border} bg-black/40 p-2`}
                    >
                      <p className="truncate font-mono text-[11px] font-medium text-green-300 sm:text-xs">
                        {alert?.id || `alert-${index + 1}`}
                      </p>
                      <p className={`font-mono text-[10px] uppercase sm:text-[11px] ${style.text}`}>
                        {alert?.severity || "info"}
                      </p>
                      <p className="mt-1 font-mono text-[11px] text-gray-300 sm:text-xs">
                        {alert?.message || "No alert message provided"}
                      </p>
                      <p className="mt-1 font-mono text-[10px] text-gray-500 sm:text-[11px]">
                        {formatTimestamp(alert?.timestampMs)}
                      </p>
                    </div>
                  );
                })
              )}
            </div>
          </motion.section>
        )}
      </AnimatePresence>
    </>
  );
}
