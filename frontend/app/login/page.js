"use client";

import { useEffect, useState } from "react";
import { useRouter } from "next/navigation";
import Link from "next/link";
import { login } from "../lib/apiClient";
import { useAuth } from "../providers/AuthProvider";

export default function LoginPage() {
  const router = useRouter();
  const { isAuthenticated, isLoading: authLoading, startOtpFlow } = useAuth();

  const [email, setEmail] = useState("");
  const [password, setPassword] = useState("");
  const [error, setError] = useState("");
  const [submitting, setSubmitting] = useState(false);

  useEffect(() => {
    if (!authLoading && isAuthenticated) {
      router.replace("/");
    }
  }, [authLoading, isAuthenticated, router]);

  const onSubmit = async (event) => {
    event.preventDefault();
    setError("");
    setSubmitting(true);

    try {
      const response = await login(email.trim(), password);
      startOtpFlow(email.trim());
      router.push("/verify-otp");
    } catch (err) {
      setError(err.message || "Login failed");
    } finally {
      setSubmitting(false);
    }
  };

  return (
    <main className="relative mx-auto flex min-h-screen w-full max-w-md flex-col justify-center px-6">
      <p className="mb-2 font-display text-[10px] font-bold uppercase tracking-[0.4em] text-accent/60">
        {"// SECURE_ACCESS // TIER_01"}
      </p>
      <h1 className="hud-title mb-2 text-3xl">Authenticate</h1>
      <p className="mb-6 font-mono text-[11px] uppercase tracking-[0.2em] text-foreground/60">
        &gt; Submit credentials to initiate OTP handshake.
      </p>

      <form
        onSubmit={onSubmit}
        className="corner-bracket-full space-y-4 rounded-xl border border-accent/40 bg-hud-panel p-5 inset-shadow-sm inset-shadow-accent/15"
      >
        <label className="block font-display text-[10px] font-bold uppercase tracking-[0.3em] text-accent">
          Email
          <input
            type="email"
            value={email}
            onChange={(event) => setEmail(event.target.value)}
            className="hud-input mt-2 w-full rounded-md px-3 py-2 text-sm normal-case"
            autoComplete="email"
            required
          />
        </label>

        <label className="block font-display text-[10px] font-bold uppercase tracking-[0.3em] text-accent">
          Password
          <input
            type="password"
            value={password}
            onChange={(event) => setPassword(event.target.value)}
            className="hud-input mt-2 w-full rounded-md px-3 py-2 text-sm normal-case"
            autoComplete="current-password"
            required
          />
        </label>

        {error ? (
          <p className="text-glow-danger font-mono text-[11px] uppercase tracking-wider text-danger">
            ! {error}
          </p>
        ) : null}

        <button
          type="submit"
          disabled={submitting}
          className="hud-button w-full rounded-md px-4 py-2 text-sm"
        >
          {submitting ? "Signing in..." : "Continue >> OTP"}
        </button>
      </form>

      <p className="mt-4 text-center font-mono text-[11px] uppercase tracking-[0.2em] text-foreground/50">
        &gt; No credentials on record?{" "}
        <Link
          href="/register"
          className="text-accent underline underline-offset-4 hover:text-glow"
        >
          REQUEST ACCESS
        </Link>
      </p>
    </main>
  );
}
