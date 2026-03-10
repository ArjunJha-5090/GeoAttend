#pragma once
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// ============================================================
// Notification.h — Abstract base class (Polymorphism)
// Defines the interface for all notification types
// ============================================================

// Abstract class with pure virtual function
class Notification {
public:
  virtual ~Notification() = default;

  // Pure virtual function — MUST be overridden by derived classes
  virtual void sendNotification(const std::string &message) = 0;
};

// ============================================================
// EmailNotification — Sends notification via email (simulated)
// ============================================================
class EmailNotification : public Notification {
private:
  std::string emailAddress;

public:
  EmailNotification(const std::string &email);
  void sendNotification(const std::string &message) override;
};

// ============================================================
// SMSNotification — Sends notification via SMS (simulated)
// ============================================================
class SMSNotification : public Notification {
private:
  std::string phoneNumber;

public:
  SMSNotification(const std::string &phone);
  void sendNotification(const std::string &message) override;
};

// ============================================================
// NotificationManager — Uses polymorphism to send warnings
// Triggers when attendance drops below 80%
// ============================================================
class NotificationManager {
private:
  std::vector<std::unique_ptr<Notification>> notifiers;

public:
  NotificationManager();
  ~NotificationManager();

  // Add a notification channel
  void addNotifier(std::unique_ptr<Notification> notifier);

  // Check percentage & send warning if below threshold
  std::string checkAndNotify(const std::string &studentName, double percentage);
};
