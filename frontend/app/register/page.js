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
        <div className="rounded-xl border border-gray-700 bg-black/30 p-5 text-center">
          <h1 className="mb-2 text-2xl font-semibold">Account created!</h1>
          <p className="mb-6 text-sm text-gray-300">
            Your account for <span className="text-white">{email}</span> is ready.
          </p>
          <Link
            href="/login"
            className="inline-block w-full rounded-md bg-white px-4 py-2 font-medium text-black text-center"
          >
            Go to Sign In
          </Link>
        </div>
      </main>
    );
  }

  return (
    <main className="mx-auto flex min-h-screen w-full max-w-md flex-col justify-center px-6">
      <h1 className="mb-2 text-2xl font-semibold">Create an account</h1>
      <p className="mb-6 text-sm text-gray-300">Enter your details to get started.</p>

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
            autoComplete="new-password"
            required
          />
          <span className="mt-2 block text-xs text-gray-500 leading-relaxed">
            Password must be at least 10 characters and contain at least one
            special character: <span className="text-gray-300">! @ # $</span>
          </span>
        </label>

        <label className="block text-sm">
          Confirm password
          <input
            type="password"
            value={confirmPassword}
            onChange={(event) => setConfirmPassword(event.target.value)}
            className="mt-1 w-full rounded-md border border-gray-700 bg-black px-3 py-2"
            autoComplete="new-password"
            required
          />
        </label>

        {error ? <p className="text-sm text-red-400">{error}</p> : null}

        <button
          type="submit"
          disabled={submitting}
          className="w-full rounded-md bg-white px-4 py-2 font-medium text-black disabled:opacity-60"
        >
          {submitting ? "Creating account..." : "Create account"}
        </button>
      </form>

      <p className="mt-4 text-center text-sm text-gray-400">
        Already have an account?{" "}
        <Link href="/login" className="text-white underline underline-offset-2">
          Sign in
        </Link>
      </p>
    </main>
  );
}