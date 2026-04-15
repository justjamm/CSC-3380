"use client";

import { createContext, useContext, useEffect, useMemo, useState } from "react";

const ACCESS_TOKEN_KEY = "auth.accessToken";
const REFRESH_TOKEN_KEY = "auth.refreshToken";
const PENDING_EMAIL_KEY = "auth.pendingEmail";

const AuthContext = createContext(null);

export function AuthProvider({ children }) {
  const [accessToken, setAccessToken] = useState(null);
  const [refreshToken, setRefreshToken] = useState(null);
  const [pendingEmail, setPendingEmail] = useState("");
  const [isLoading, setIsLoading] = useState(true);

  useEffect(() => {
    const savedAccessToken = window.localStorage.getItem(ACCESS_TOKEN_KEY);
    const savedRefreshToken = window.localStorage.getItem(REFRESH_TOKEN_KEY);
    const savedPendingEmail = window.localStorage.getItem(PENDING_EMAIL_KEY);

    setAccessToken(savedAccessToken || null);
    setRefreshToken(savedRefreshToken || null);
    setPendingEmail(savedPendingEmail || "");
    setIsLoading(false);
  }, []);

  const startOtpFlow = (email) => {
    setPendingEmail(email);
    window.localStorage.setItem(PENDING_EMAIL_KEY, email);
  };

  const completeAuth = ({ token, accessToken: nextAccessToken, refreshToken: nextRefreshToken }) => {
    const resolvedAccessToken = token || nextAccessToken || null;
    const resolvedRefreshToken = nextRefreshToken || null;

    setAccessToken(resolvedAccessToken);
    setRefreshToken(resolvedRefreshToken);
    setPendingEmail("");

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

    window.localStorage.removeItem(PENDING_EMAIL_KEY);
  };

  const logout = () => {
    setAccessToken(null);
    setRefreshToken(null);
    setPendingEmail("");
    window.localStorage.removeItem(ACCESS_TOKEN_KEY);
    window.localStorage.removeItem(REFRESH_TOKEN_KEY);
    window.localStorage.removeItem(PENDING_EMAIL_KEY);
  };

  const value = useMemo(
    () => ({
      accessToken,
      refreshToken,
      pendingEmail,
      isLoading,
      isAuthenticated: Boolean(accessToken),
      startOtpFlow,
      completeAuth,
      logout,
    }),
    [accessToken, refreshToken, pendingEmail, isLoading]
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
