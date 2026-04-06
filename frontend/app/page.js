/* eslint-disable @next/next/no-img-element */
"use client";

import { useCallback, useEffect, useRef, useState } from "react";
import { useRouter } from "next/navigation";
import ProtectedRoute from "./components/ProtectedRoute";
import {
  getAlerts,
  getDevices,
  getRealtimeWebSocketUrl,
  selectCamera,
} from "./lib/apiClient";
import { useAuth } from "./providers/AuthProvider";
import useMjpegStream from "./useMjpegStream";

function DashboardPage() {
  const router = useRouter();
  const { accessToken, logout } = useAuth();

  const [devices, setDevices] = useState([]);
  const [alerts, setAlerts] = useState([]);
  const [selectedCamera, setSelectedCamera] = useState("");
  const [error, setError] = useState("");
  const [loading, setLoading] = useState(true);
  const [wsStatus, setWsStatus] = useState("offline");
  const [lastRealtimeAt, setLastRealtimeAt] = useState(0);
  const reconnectAttemptRef = useRef(0);
  const selectedCameraRef = useRef("");

  selectedCameraRef.current = selectedCamera;

  const imgRef = useMjpegStream(selectedCamera, accessToken);
  const displayWsStatus = accessToken ? wsStatus : "offline";

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
    setAlerts(nextAlerts);

    return nextDevices;
  }, [accessToken]);

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
            setError(err.message || "Failed to select default camera");
          }
        }
      } catch (err) {
        if (!active) {
          return;
        }
        setError(err.message || "Failed to load dashboard data");
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
        }
        if (typeof data.selectedCameraId === "string" && data.selectedCameraId) {
          setSelectedCamera(data.selectedCameraId);
        }
        return;
      }

      if (type === "alert") {
        if (Array.isArray(data.alerts)) {
          setAlerts(data.alerts);
        }
        return;
      }

      if (type === "error" && data?.message) {
        setError(data.message);
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
  }, [accessToken]);

  const onCameraChange = async (event) => {
    const nextCameraId = event.target.value;
    setSelectedCamera(nextCameraId);
    setError("");

    try {
      await selectCamera(nextCameraId, accessToken);
    } catch (err) {
      setError(err.message || "Failed to select camera");
    }
  };

  const onLogout = () => {
    logout();
    router.replace("/login");
  };

  return (
    <main className="flex min-h-screen flex-col items-center p-8">
      <div className="mb-6 flex w-full max-w-4xl items-center justify-between">
        <h1 className="text-3xl font-bold">IoT EDR Hub</h1>
        <button
          type="button"
          onClick={onLogout}
          className="rounded-md border border-gray-600 px-3 py-1 text-sm"
        >
          Logout
        </button>
      </div>

      <div className="mb-3 flex w-full max-w-4xl items-center justify-between text-xs text-gray-300">
        <p>Realtime: {displayWsStatus}</p>
        <p>
          Last update: {lastRealtimeAt ? new Date(lastRealtimeAt).toLocaleTimeString() : "waiting"}
        </p>
      </div>

      {error ? <p className="mb-4 w-full max-w-4xl text-sm text-red-400">{error}</p> : null}

      <div className="grid w-full max-w-4xl gap-6 md:grid-cols-[2fr_1fr]">
        <section className="rounded-lg border border-gray-700 p-4">
          <h2 className="mb-3 text-lg font-medium">Camera Stream</h2>
          {loading ? <p className="mb-3 text-sm text-gray-300">Loading devices...</p> : null}
          <select
            value={selectedCamera}
            onChange={onCameraChange}
            disabled={devices.length === 0}
            className="mb-4 w-full rounded-lg border border-gray-700 bg-black px-4 py-2 text-white"
          >
            {devices.length === 0 ? (
              <option value="">No devices</option>
            ) : (
              devices.map((device) => (
                <option key={device.id} value={device.id}>
                  {device.label || device.id}
                </option>
              ))
            )}
          </select>

          <img
            ref={imgRef}
            alt="Camera Stream"
            className="w-full rounded-lg border border-gray-700"
          />
        </section>

        <aside className="rounded-lg border border-gray-700 p-4">
          <h2 className="mb-3 text-lg font-medium">Alerts</h2>
          {alerts.length === 0 ? (
            <p className="text-sm text-gray-300">No active alerts.</p>
          ) : (
            <ul className="space-y-3">
              {alerts.map((alert) => (
                <li key={alert.id} className="rounded-md border border-gray-700 p-2 text-sm">
                  <p className="font-medium">{alert.id}</p>
                  <p className="text-xs uppercase text-yellow-300">{alert.severity || "info"}</p>
                  <p className="text-xs text-gray-300">{alert.message}</p>
                </li>
              ))}
            </ul>
          )}
        </aside>
      </div>
    </main>
  );
}

export default function HomePage() {
  return (
    <ProtectedRoute>
      <DashboardPage />
    </ProtectedRoute>
  );
}
