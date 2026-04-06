"use client";

// All browser calls use same-origin /api and are forwarded by next.config rewrites.
const API_BASE_URL = "/api";

function resolveUrl(path) {
  if (/^https?:\/\//.test(path)) {
    return path;
  }

  const normalizedPath = path.startsWith("/") ? path : `/${path}`;
  return `${API_BASE_URL}${normalizedPath}`;
}

function normalizePath(pathname) {
  const trimmed = pathname.replace(/\/+$/, "");
  return trimmed ? trimmed : "";
}

async function parseErrorResponse(response) {
  const contentType = response.headers.get("content-type") || "";
  if (contentType.includes("application/json")) {
    const data = await response.json();
    return data.message || data.error || `Request failed with status ${response.status}`;
  }

  const text = await response.text();
  return text || `Request failed with status ${response.status}`;
}

async function request(path, options = {}) {
  const {
    method = "GET",
    token,
    body,
    headers = {},
    signal,
    responseType = "json",
  } = options;

  const requestHeaders = { ...headers };
  if (token) {
    requestHeaders.Authorization = `Bearer ${token}`;
  }
  if (body !== undefined) {
    requestHeaders["Content-Type"] = "application/json";
  }

  const response = await fetch(resolveUrl(path), {
    method,
    headers: requestHeaders,
    body: body !== undefined ? JSON.stringify(body) : undefined,
    signal,
  });

  if (!response.ok) {
    const message = await parseErrorResponse(response);
    const error = new Error(message);
    error.status = response.status;
    throw error;
  }

  if (responseType === "blob") {
    return response.blob();
  }
  if (responseType === "text") {
    return response.text();
  }

  const contentType = response.headers.get("content-type") || "";
  if (contentType.includes("application/json")) {
    return response.json();
  }

  const text = await response.text();
  try {
    return JSON.parse(text);
  } catch {
    return { raw: text };
  }
}

export function login(username, password) {
  return request("/auth/login", {
    method: "POST",
    body: { username, password },
  });
}

export function verifyOtp(username, otp) {
  return request("/auth/verify-otp", {
    method: "POST",
    body: { username, otp },
  });
}

export function selectCamera(cameraId, token) {
  return request("/camera/select", {
    method: "POST",
    token,
    body: { cameraId },
  });
}

export function getDevices(token) {
  return request("/devices", { token });
}

export function getAlerts(token) {
  return request("/alerts", { token });
}

export function getStreamFrame(cameraId, token, signal) {
  return request(`/stream/${encodeURIComponent(cameraId)}?t=${Date.now()}`, {
    token,
    signal,
    responseType: "blob",
  });
}

export function getMjpegUrl(cameraId) {
  return resolveUrl(`/mjpeg/${encodeURIComponent(cameraId)}`);
}

export function getRealtimeWebSocketUrl(token) {
  const encodedToken = encodeURIComponent(token);

  if (typeof window !== "undefined") {
    const protocol = window.location.protocol === "https:" ? "wss:" : "ws:";
    return `${protocol}//${window.location.host}/api/ws?token=${encodedToken}`;
  }

  const configuredApiUrl = process.env.NEXT_PUBLIC_API_URL;
  if (configuredApiUrl && /^https?:\/\//.test(configuredApiUrl)) {
    const wsUrl = new URL(configuredApiUrl);
    wsUrl.protocol = wsUrl.protocol === "https:" ? "wss:" : "ws:";
    wsUrl.pathname = `${normalizePath(wsUrl.pathname)}/ws`;
    wsUrl.search = `token=${encodedToken}`;
    return wsUrl.toString();
  }

  return `ws://localhost:8085/ws?token=${encodedToken}`;
}
