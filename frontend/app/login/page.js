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
    <main className="mx-auto flex min-h-screen w-full max-w-md flex-col justify-center px-6">
      <h1 className="mb-2 text-2xl font-semibold">Sign in</h1>
      <p className="mb-6 text-sm text-gray-300">Use your email and password to start OTP flow.</p>

      <form onSubmit={onSubmit} className="space-y-4 rounded-xl border border-gray-700 bg-black/30 p-5">
        <label className="block text-sm">
          Email
          <input
            type="email"
            value={email}
            onChange={(event) => setEmail(event.target.value)}
            className="mt-1 w-full rounded-md border border-gray-700 bg-black px-3 py-2"
            autoComplete="email"
            required
          />
        </label>

        <label className="block text-sm">
          Password
          <input
            type="password"
            value={password}
            onChange={(event) => setPassword(event.target.value)}
            className="mt-1 w-full rounded-md border border-gray-700 bg-black px-3 py-2"
            autoComplete="current-password"
            required
          />
        </label>

        {error ? <p className="text-sm text-red-400">{error}</p> : null}

        <button
          type="submit"
          disabled={submitting}
          className="w-full rounded-md bg-white px-4 py-2 font-medium text-black disabled:opacity-60"
        >
          {submitting ? "Signing in..." : "Continue to OTP"}
        </button>
      </form>

      <p className="mt-4 text-center text-sm text-gray-400">
        Don&apos;t have an account?{" "}
        <Link href="/register" className="text-white underline underline-offset-2">
          Create one
        </Link>
      </p>
    </main>
  );
}
