"use client";

import { useCallback, useEffect, useRef, useState } from "react";
import { useRouter } from "next/navigation";
import { AnimatePresence } from "framer-motion";
import ProtectedRoute from "./components/ProtectedRoute";
import VideoViewport from "./components/VideoViewport";
import StatusBar from "./components/StatusBar";
import AlertsOverlay from "./components/AlertsOverlay";
import LogoutButton from "./components/LogoutButton";
import CameraMapToggle from "./components/CameraMapToggle";
import CameraMapPanel from "./components/CameraMapPanel";
import SettingsPanel from "./components/SettingsPanel";
import {
  getAlerts,
  getDevices,
  getRealtimeWebSocketUrl,
  selectCamera,
} from "./lib/apiClient";
import { useAuth } from "./providers/AuthProvider";
import useMjpegStream from "./useMjpegStream";

function resolveErrorMessage(value, fallback) {
  if (typeof value === "string" && value.trim()) {
    return value;
  }

  if (value && typeof value === "object") {
    if (typeof value.message === "string" && value.message.trim()) {
      return value.message;
    }
    if (typeof value.error === "string" && value.error.trim()) {
      return value.error;
    }
    if (value.error && typeof value.error === "object") {
      if (typeof value.error.message === "string" && value.error.message.trim()) {
        return value.error.message;
      }
    }
  }

  return fallback;
}

function getAlertTimestamp(alert) {
  return Number.isFinite(alert?.timestampMs) ? alert.timestampMs : -1;
}

function isActiveMotionAlert(alert) {
  if (!alert || typeof alert !== "object") {
    return false;
  }

  const cameraId = typeof alert.cameraId === "string" ? alert.cameraId.trim() : "";
  if (!cameraId) {
    return false;
  }

  if (alert.active === true) {
    return true;
  }

  const state = typeof alert.state === "string" ? alert.state.toLowerCase() : "";
  if (state === "active") {
    return true;
  }

  const source = typeof alert.source === "string" ? alert.source.toLowerCase() : "";
  const severity = typeof alert.severity === "string" ? alert.severity.toLowerCase() : "";
  return source === "motion-detection" && (severity === "high" || severity === "critical");
}

function findNewestActiveMotionAlert(alerts) {
  if (!Array.isArray(alerts) || alerts.length === 0) {
    return null;
  }

  let newest = null;
  for (const alert of alerts) {
    if (!isActiveMotionAlert(alert)) {
      continue;
    }

    if (!newest || getAlertTimestamp(alert) > getAlertTimestamp(newest)) {
      newest = alert;
    }
  }

  return newest;
}

function getAlertSwitchKey(alert) {
  const id = typeof alert?.id === "string" ? alert.id : "unknown";
  const cameraId = typeof alert?.cameraId === "string" ? alert.cameraId : "";
  const timestamp = getAlertTimestamp(alert);
  const state = typeof alert?.state === "string" ? alert.state : "";
  return `${id}|${cameraId}|${timestamp}|${state}`;
}

function DashboardPage() {
  const router = useRouter();
  const { accessToken, logout } = useAuth();

  const [devices, setDevices] = useState([]);
  const [alerts, setAlerts] = useState([]);
  const [selectedCamera, setSelectedCamera] = useState("");
  const [autoSwitchEnabled, setAutoSwitchEnabled] = useState(false);
  const [error, setError] = useState("");
  const [loading, setLoading] = useState(true);
  const [wsStatus, setWsStatus] = useState("offline");
  const [lastRealtimeAt, setLastRealtimeAt] = useState(0);
  const [isMapOpen, setIsMapOpen] = useState(false);
  const reconnectAttemptRef = useRef(0);
  const selectedCameraRef = useRef("");
  const autoSwitchEnabledRef = useRef(false);
  const devicesRef = useRef([]);
  const lastAutoSwitchKeyRef = useRef("");

  selectedCameraRef.current = selectedCamera;
  autoSwitchEnabledRef.current = autoSwitchEnabled;
  devicesRef.current = devices;

  const imgRef = useMjpegStream(selectedCamera, accessToken);
  const displayWsStatus = accessToken ? wsStatus : "offline";

  const selectCameraWithApi = useCallback(
    async (cameraId, fallbackError = "Failed to select camera") => {
      setSelectedCamera(cameraId);
      setError("");

      try {
        await selectCamera(cameraId, accessToken);
      } catch (err) {
        setError(resolveErrorMessage(err, fallbackError));
      }
    },
    [accessToken],
  );

  const tryAutoSwitchFromAlerts = useCallback(
    (nextAlerts) => {
      if (!autoSwitchEnabledRef.current || !accessToken) {
        return;
      }

      const nextAlert = findNewestActiveMotionAlert(nextAlerts);
      if (!nextAlert) {
        return;
      }

      const nextCameraId = nextAlert.cameraId.trim();
      const knownCameraIds = new Set(devicesRef.current.map((device) => device?.id).filter(Boolean));
      if (knownCameraIds.size > 0 && !knownCameraIds.has(nextCameraId)) {
        return;
      }

      const switchKey = getAlertSwitchKey(nextAlert);
      if (switchKey === lastAutoSwitchKeyRef.current) {
        return;
      }
      lastAutoSwitchKeyRef.current = switchKey;

      if (selectedCameraRef.current === nextCameraId) {
        return;
      }

      selectCameraWithApi(nextCameraId, "Failed to auto-switch camera on motion");
    },
    [accessToken, selectCameraWithApi],
  );

  const applyIncomingAlerts = useCallback(
    (nextAlerts) => {
      setAlerts(nextAlerts);
      tryAutoSwitchFromAlerts(nextAlerts);
    },
    [tryAutoSwitchFromAlerts],
  );

  useEffect(() => {
    if (!autoSwitchEnabled) {
      return;
    }
    tryAutoSwitchFromAlerts(alerts);
  }, [autoSwitchEnabled, alerts, tryAutoSwitchFromAlerts]);

  const loadSnapshot = useCallback(async () => {
    const [devicesResponse, alertsResponse] = await Promise.all([
      getDevices(accessToken),
      getAlerts(accessToken),
    ]);

    const nextDevices = Array.isArray(devicesResponse?.devices)
      ? devicesResponse.devices
      : [];
    const nextAlerts = Array.isArray(alertsResponse?.alerts)
      ? alertsResponse.alerts
      : [];

    setDevices(nextDevices);
    applyIncomingAlerts(nextAlerts);

    return nextDevices;
  }, [accessToken, applyIncomingAlerts]);

  useEffect(() => {
    if (!accessToken) {
      return;
    }

    let active = true;
    const hydrateDashboard = async () => {
      try {
        const nextDevices = await loadSnapshot();
        if (!active) {
          return;
        }

        setError("");

        if (!selectedCameraRef.current && nextDevices.length > 0) {
          const defaultCameraId = nextDevices[0].id;
          setSelectedCamera(defaultCameraId);
          try {
            await selectCamera(defaultCameraId, accessToken);
          } catch (err) {
            if (!active) {
              return;
            }
            setError(resolveErrorMessage(err, "Failed to select default camera"));
          }
        }
      } catch (err) {
        if (!active) {
          return;
        }
        setError(resolveErrorMessage(err, "Failed to load dashboard data"));
      } finally {
        if (active) {
          setLoading(false);
        }
      }
    };

    hydrateDashboard();

    return () => {
      active = false;
    };
  }, [accessToken, loadSnapshot]);

  useEffect(() => {
    if (!accessToken) {
      return;
    }

    let active = true;
    let pollTimer = null;

    const pollAlerts = async () => {
      try {
        const alertsResponse = await getAlerts(accessToken);
        if (!active) {
          return;
        }
        if (Array.isArray(alertsResponse?.alerts)) {
          applyIncomingAlerts(alertsResponse.alerts);
        }
      } catch {
        // Keep polling even if a refresh attempt fails.
      }

      if (active) {
        pollTimer = setTimeout(pollAlerts, 3000);
      }
    };

    pollTimer = setTimeout(pollAlerts, 3000);

    return () => {
      active = false;
      if (pollTimer) {
        clearTimeout(pollTimer);
      }
    };
  }, [accessToken, applyIncomingAlerts]);

  useEffect(() => {
    if (!accessToken) {
      return;
    }

    let socket = null;
    let reconnectTimer = null;
    let stopped = false;

    const handleEvent = (rawMessage) => {
      let parsed;
      try {
        parsed = JSON.parse(rawMessage);
      } catch {
        return;
      }

      const type = parsed?.type || parsed?.event;
      const data = parsed?.data || {};
      setLastRealtimeAt(Date.now());

      if (type === "device_update") {
        if (Array.isArray(data.devices)) {
          setDevices(data.devices);
          // Safety net: if no camera selected yet and devices arrived, pick the first one.
          if (!selectedCameraRef.current && data.devices.length > 0) {
            const fallbackId = data.devices[0].id;
            setSelectedCamera(fallbackId);
            selectCamera(fallbackId, accessToken).catch((err) => {
              setError(resolveErrorMessage(err, "Failed to select default camera"));
            });
          }
        }
        if (typeof data.selectedCameraId === "string" && data.selectedCameraId) {
          setSelectedCamera(data.selectedCameraId);
        }
        return;
      }

      if (type === "alert") {
        if (Array.isArray(data.alerts)) {
          applyIncomingAlerts(data.alerts);
        }
        return;
      }

      if (type === "error") {
        const realtimeErrorMessage = resolveErrorMessage(data, "");
        if (realtimeErrorMessage) {
          setError(realtimeErrorMessage);
        }
      }
    };

    const scheduleReconnect = () => {
      if (stopped) {
        return;
      }
      reconnectAttemptRef.current += 1;
      const retryDelayMs = Math.min(reconnectAttemptRef.current * 1200, 8000);
      reconnectTimer = setTimeout(() => {
        if (!stopped) {
          setWsStatus("reconnecting");
          connect();
        }
      }, retryDelayMs);
    };

    const connect = () => {
      if (stopped) {
        return;
      }

      try {
        socket = new WebSocket(getRealtimeWebSocketUrl(accessToken));
      } catch {
        scheduleReconnect();
        return;
      }

      socket.onopen = () => {
        if (stopped) {
          socket.close();
          return;
        }
        reconnectAttemptRef.current = 0;
        setWsStatus("connected");
      };

      socket.onmessage = (event) => {
        if (typeof event.data === "string") {
          handleEvent(event.data);
        }
      };

      socket.onerror = () => {
        if (!stopped) {
          setWsStatus("error");
        }
      };

      socket.onclose = () => {
        if (!stopped) {
          scheduleReconnect();
        }
      };
    };

    connect();

    return () => {
      stopped = true;
      if (reconnectTimer) {
        clearTimeout(reconnectTimer);
      }
      if (socket && socket.readyState === WebSocket.OPEN) {
        socket.close();
      }
    };
  }, [accessToken, applyIncomingAlerts]);

  const onCameraChange = async (cameraId) => {
    await selectCameraWithApi(cameraId, "Failed to select camera");
  };

  const onLogout = () => {
    logout();
    router.replace("/login");
  };

  return (
    <>
      <VideoViewport
        imgRef={imgRef}
        selectedCamera={selectedCamera}
        loading={loading}
      />

      <StatusBar
        wsStatus={displayWsStatus}
        lastRealtimeAt={lastRealtimeAt}
        error={error}
      />

      <AlertsOverlay alerts={alerts} />

      <SettingsPanel
        autoSwitchEnabled={autoSwitchEnabled}
        onAutoSwitchToggle={setAutoSwitchEnabled}
      />

      <LogoutButton onLogout={onLogout} />

      <CameraMapToggle
        isOpen={isMapOpen}
        onToggle={() => setIsMapOpen((prev) => !prev)}
      />

      <AnimatePresence>
        {isMapOpen && (
          <CameraMapPanel
            devices={devices}
            selectedCamera={selectedCamera}
            onCameraSelect={onCameraChange}
          />
        )}
      </AnimatePresence>
    </>
  );
}

export default function HomePage() {
  return (
    <ProtectedRoute>
      <DashboardPage />
    </ProtectedRoute>
  );
}
