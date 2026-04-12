const mongoose = require("mongoose");
const bcrypt = require("bcryptjs");

const userSchema = new mongoose.Schema({
  email: { type: String, required: true, unique: true, trim: true, lowercase: true },
  passwordHash: { type: String, required: true },
  role: { type: String, default: "operator" },
  otpCode: { type: String, default: null },
  otpExpiresAt: { type: Date, default: null },
}, { timestamps: true });

userSchema.methods.setPassword = async function (plaintext) {
  this.passwordHash = await bcrypt.hash(plaintext, 12);
};

userSchema.methods.checkPassword = function (plaintext) {
  return bcrypt.compare(plaintext, this.passwordHash);
};

module.exports = mongoose.model("User", userSchema);
