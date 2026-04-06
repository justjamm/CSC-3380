"use client";

import { createContext, useContext, useEffect, useMemo, useState } from "react";

const ACCESS_TOKEN_KEY = "auth.accessToken";
const REFRESH_TOKEN_KEY = "auth.refreshToken";
const PENDING_USERNAME_KEY = "auth.pendingUsername";

const AuthContext = createContext(null);

export function AuthProvider({ children }) {
  const [accessToken, setAccessToken] = useState(null);
  const [refreshToken, setRefreshToken] = useState(null);
  const [pendingUsername, setPendingUsername] = useState("");
  const [isLoading, setIsLoading] = useState(true);

  useEffect(() => {
    const savedAccessToken = window.localStorage.getItem(ACCESS_TOKEN_KEY);
    const savedRefreshToken = window.localStorage.getItem(REFRESH_TOKEN_KEY);
    const savedPendingUsername = window.localStorage.getItem(PENDING_USERNAME_KEY);

    setAccessToken(savedAccessToken || null);
    setRefreshToken(savedRefreshToken || null);
    setPendingUsername(savedPendingUsername || "");
    setIsLoading(false);
  }, []);

  const startOtpFlow = (username) => {
    setPendingUsername(username);
    window.localStorage.setItem(PENDING_USERNAME_KEY, username);
  };

  const completeAuth = ({ token, accessToken: nextAccessToken, refreshToken: nextRefreshToken }) => {
    const resolvedAccessToken = token || nextAccessToken || null;
    const resolvedRefreshToken = nextRefreshToken || null;

    setAccessToken(resolvedAccessToken);
    setRefreshToken(resolvedRefreshToken);
    setPendingUsername("");

    if (resolvedAccessToken) {
      window.localStorage.setItem(ACCESS_TOKEN_KEY, resolvedAccessToken);
    } else {
      window.localStorage.removeItem(ACCESS_TOKEN_KEY);
    }

    if (resolvedRefreshToken) {
      window.localStorage.setItem(REFRESH_TOKEN_KEY, resolvedRefreshToken);
    } else {
      window.localStorage.removeItem(REFRESH_TOKEN_KEY);
    }

    window.localStorage.removeItem(PENDING_USERNAME_KEY);
  };

  const logout = () => {
    setAccessToken(null);
    setRefreshToken(null);
    setPendingUsername("");
    window.localStorage.removeItem(ACCESS_TOKEN_KEY);
    window.localStorage.removeItem(REFRESH_TOKEN_KEY);
    window.localStorage.removeItem(PENDING_USERNAME_KEY);
  };

  const value = useMemo(
    () => ({
      accessToken,
      refreshToken,
      pendingUsername,
      isLoading,
      isAuthenticated: Boolean(accessToken),
      startOtpFlow,
      completeAuth,
      logout,
    }),
    [accessToken, refreshToken, pendingUsername, isLoading]
  );

  return <AuthContext.Provider value={value}>{children}</AuthContext.Provider>;
}

export function useAuth() {
  const context = useContext(AuthContext);
  if (!context) {
    throw new Error("useAuth must be used within AuthProvider");
  }
  return context;
}
