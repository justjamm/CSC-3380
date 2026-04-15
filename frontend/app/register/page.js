"use client";

import { useEffect, useState } from "react";
import { useRouter } from "next/navigation";
import Link from "next/link";
import { register } from "../lib/apiClient";
import { useAuth } from "../providers/AuthProvider";

const PASSWORD_REGEX = /^(?=.*[!@#$])[A-Za-z0-9!@#$]{10,}$/;

export default function RegisterPage() {
  const router = useRouter();
  const { isAuthenticated, isLoading: authLoading } = useAuth();

  const [email, setEmail] = useState("");
  const [password, setPassword] = useState("");
  const [confirmPassword, setConfirmPassword] = useState("");
  const [error, setError] = useState("");
  const [submitting, setSubmitting] = useState(false);
  const [success, setSuccess] = useState(false);

  useEffect(() => {
    if (!authLoading && isAuthenticated) {
      router.replace("/");
    }
  }, [authLoading, isAuthenticated, router]);

  const onSubmit = async (event) => {
    event.preventDefault();
    setError("");

    if (!PASSWORD_REGEX.test(password)) {
      setError("Password does not meet the requirements listed below.");
      return;
    }
    if (password !== confirmPassword) {
      setError("Passwords do not match");
      return;
    }

    setSubmitting(true);
    try {
      await register(email.trim(), password);
      setSuccess(true);
    } catch (err) {
      setError(err.message || "Registration failed");
    } finally {
      setSubmitting(false);
    }
  };

  if (success) {
    return (
      <main className="mx-auto flex min-h-screen w-full max-w-md flex-col justify-center px-6">
        <div className="corner-bracket-full rounded-xl border border-accent/40 bg-hud-panel p-5 text-center inset-shadow-sm inset-shadow-accent/15">
          <p className="mb-2 font-display text-[10px] font-bold uppercase tracking-[0.4em] text-accent/60">
            {"// ENROLLMENT // COMPLETE"}
          </p>
          <h1 className="hud-title mb-3 text-2xl">Identity Logged</h1>
          <p className="mb-6 font-mono text-[11px] uppercase tracking-[0.2em] text-foreground/70">
            &gt; Profile for <span className="text-accent">{email}</span> is active.
          </p>
          <Link
            href="/login"
            className="hud-button inline-block w-full rounded-md px-4 py-2 text-sm text-center"
          >
            Proceed &gt;&gt; Sign In
          </Link>
        </div>
      </main>
    );
  }

  return (
    <main className="relative mx-auto flex min-h-screen w-full max-w-md flex-col justify-center px-6">
      <p className="mb-2 font-display text-[10px] font-bold uppercase tracking-[0.4em] text-accent/60">
        {"// ENROLLMENT // NEW_OPERATOR"}
      </p>
      <h1 className="hud-title mb-2 text-3xl">Register</h1>
      <p className="mb-6 font-mono text-[11px] uppercase tracking-[0.2em] text-foreground/60">
        &gt; Provide identifiers to request clearance.
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
            autoComplete="new-password"
            required
          />
          <span className="mt-2 block font-mono text-[10px] uppercase tracking-[0.15em] text-foreground/50 leading-relaxed">
            &gt; MIN 10 CHARS, REQUIRE:{" "}
            <span className="text-accent">! @ # $</span>
          </span>
        </label>

        <label className="block font-display text-[10px] font-bold uppercase tracking-[0.3em] text-accent">
          Confirm password
          <input
            type="password"
            value={confirmPassword}
            onChange={(event) => setConfirmPassword(event.target.value)}
            className="hud-input mt-2 w-full rounded-md px-3 py-2 text-sm normal-case"
            autoComplete="new-password"
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
          {submitting ? "Creating account..." : "Create Account"}
        </button>
      </form>

      <p className="mt-4 text-center font-mono text-[11px] uppercase tracking-[0.2em] text-foreground/50">
        &gt; Previously enrolled?{" "}
        <Link
          href="/login"
          className="text-accent underline underline-offset-4 hover:text-glow"
        >
          SIGN IN
        </Link>
      </p>
    </main>
  );
}
