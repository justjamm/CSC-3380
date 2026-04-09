const express = require("express");
const app = express();
const PORT = process.env.PORT || 8080;

app.get("/health", (req, res) => {
  res.json({ status: "ok", service: "edr-middleware" });
});

app.post("/auth/login", async (req, res) => {
  try {
    const response = await fetch(`${AUTH_SERVICE_URL}/auth/login`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(req.body),
    });
    const data = await response.json();
    res.status(response.status).json(data);
  } catch (err) {
    res.status(502).json({ message: "Auth service unavailable" });
  }
});

app.post("/auth/verify-otp", async (req, res) => {
  try {
    const response = await fetch(`${AUTH_SERVICE_URL}/auth/verify-otp`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(req.body),
    });
    const data = await response.json();
    res.status(response.status).json(data);
  } catch (err) {
    res.status(502).json({ message: "Auth service unavailable" });
  }
});

app.listen(PORT, () => {
  console.log(`Middleware placeholder listening on port ${PORT}`);
});
