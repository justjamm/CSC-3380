const express = require("express");
const mongoose = require("mongoose");
const jwt = require("jsonwebtoken");
const crypto = require("crypto");
const User = require("./models/User");

const app = express();
app.use(express.json());

const PORT = process.env.PORT || 3002;
const MONGO_URI = process.env.MONGO_URI || "mongodb://mongo:27017/edr";
const JWT_SECRET = process.env.JWT_SECRET || "dev-secret";
const JWT_ISSUER = process.env.JWT_ISSUER || "middleware";
const JWT_AUDIENCE = process.env.JWT_AUDIENCE || "frontend";
const OTP_EXPIRY_SECS = parseInt(process.env.OTP_EXPIRY_SECS || "300", 10);

// Simple email format validator
function isValidEmail(value) {
  return /^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(value);
}

mongoose.connect(MONGO_URI).then(async () => {
  console.log("[auth-service] Connected to MongoDB");
  // Seed a default admin if the users collection is empty
  const count = await User.countDocuments();
  if (count === 0) {
    const admin = new User({ email: "admin@example.com", role: "admin" });
    await admin.setPassword("admin123");
    await admin.save();
    console.log("[auth-service] Seeded default admin (email: admin@example.com, password: admin123)");
  }
}).catch((err) => {
  console.error("[auth-service] MongoDB error:", err.message);
  process.exit(1);
});

// POST /auth/register
app.post("/auth/register", async (req, res) => {
  const { email, password } = req.body || {};

  if (!email || !password) {
    return res.status(400).json({ message: "Email and password are required" });
  }
  if (!isValidEmail(email)) {
    return res.status(400).json({ message: "Invalid email address" });
  }
  if (password.length < 8) {
    return res.status(400).json({ message: "Password must be at least 8 characters" });
  }

  try {
    const existing = await User.findOne({ email: email.toLowerCase() });
    if (existing) {
      return res.status(409).json({ message: "An account with that email already exists" });
    }

    const user = new User({ email: email.toLowerCase(), role: "operator" });
    await user.setPassword(password);
    await user.save();

    console.log(`[auth-service] New account registered: ${email}`);
    return res.status(201).json({ message: "Account created successfully" });
  } catch (err) {
    console.error(err);
    return res.status(500).json({ message: "Internal server error" });
  }
});

// POST /auth/login
app.post("/auth/login", async (req, res) => {
  const { email, password } = req.body || {};
  if (!email || !password) {
    return res.status(400).json({ message: "Email and password are required" });
  }
  if (!isValidEmail(email)) {
    return res.status(400).json({ message: "Invalid email address" });
  }
  try {
    const user = await User.findOne({ email: email.toLowerCase() });
    if (!user || !(await user.checkPassword(password))) {
      return res.status(401).json({ message: "Invalid email or password" });
    }
    const otp = crypto.randomInt(100000, 999999).toString();
    user.otpCode = otp;
    user.otpExpiresAt = new Date(Date.now() + OTP_EXPIRY_SECS * 1000);
    await user.save();
    const nodemailer = require("nodemailer");

    const transporter = nodemailer.createTransport({
        service: "gmail",
        auth: {
            user: process.env.EMAIL_USER,
            pass: process.env.EMAIL_PASS,
        },
    });

    await transporter.sendMail({
        from: process.env.EMAIL_USER,
        to: user.email,             // you'll need to add email field to your User schema
        subject: "Your verification code",
        text: `Your code is: ${otp}. It expires in ${OTP_EXPIRY_SECS / 60} minutes.`,
    });

    return res.json({ status: "otp_required", email });
  } catch (err) {
    console.error(err);
    return res.status(500).json({ message: "Internal server error" });
  }
});

// POST /auth/verify-otp
app.post("/auth/verify-otp", async (req, res) => {
  const { email, otp } = req.body || {};
  if (!email || !otp) {
    return res.status(400).json({ message: "Email and OTP are required" });
  }
  try {
    const user = await User.findOne({ email: email.toLowerCase() });
    if (!user || !user.otpCode) {
      return res.status(401).json({ message: "No OTP pending. Please log in again." });
    }
    if (new Date() > user.otpExpiresAt) {
      user.otpCode = null; user.otpExpiresAt = null;
      await user.save();
      return res.status(401).json({ message: "OTP expired. Please log in again." });
    }
    if (user.otpCode !== otp.trim()) {
      return res.status(401).json({ message: "Invalid OTP" });
    }
    user.otpCode = null; user.otpExpiresAt = null;
    await user.save();

    const accessToken = jwt.sign(
      { userId: user._id.toString(), username: user.email, role: user.role },
      JWT_SECRET,
      {
        expiresIn: "15m",
        issuer: JWT_ISSUER,
        audience: JWT_AUDIENCE,
        subject: user._id.toString(),
      }
    );
    const refreshToken = jwt.sign(
      { userId: user._id.toString(), type: "refresh" },
      JWT_SECRET,
      {
        expiresIn: "1d",
        issuer: JWT_ISSUER,
        audience: JWT_AUDIENCE,
        subject: user._id.toString(),
      }
    );
    return res.json({ accessToken, refreshToken });
  } catch (err) {
    console.error(err);
    return res.status(500).json({ message: "Internal server error" });
  }
});

app.get("/health", (_req, res) => res.json({ status: "ok" }));
app.listen(PORT, () => console.log(`[auth-service] Listening on port ${PORT}`));
