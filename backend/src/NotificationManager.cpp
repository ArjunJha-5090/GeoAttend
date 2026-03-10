#include "../include/Notification.h"

// ============================================================
// NotificationManager.cpp — Polymorphic notification system
// ============================================================

// --- EmailNotification ---
EmailNotification::EmailNotification(const std::string &email)
    : emailAddress(email) {}

void EmailNotification::sendNotification(const std::string &message) {
  std::cout << "[EMAIL to " << emailAddress << "] " << message << "\n";
}

// --- SMSNotification ---
SMSNotification::SMSNotification(const std::string &phone)
    : phoneNumber(phone) {}

void SMSNotification::sendNotification(const std::string &message) {
  std::cout << "[SMS to " << phoneNumber << "] " << message << "\n";
}

// --- NotificationManager ---
NotificationManager::NotificationManager() {
  std::cout << "[NotificationManager] Ready.\n";
}

NotificationManager::~NotificationManager() {
  std::cout << "[NotificationManager] Shutting down.\n";
}

// Add a notification channel (email or SMS)
void NotificationManager::addNotifier(std::unique_ptr<Notification> notifier) {
  notifiers.push_back(std::move(notifier));
}

// Check attendance and send warning if below 80%
std::string NotificationManager::checkAndNotify(const std::string &studentName,
                                                double percentage) {
  if (percentage < 80.0) {
    std::string msg = "WARNING: " + studentName + "'s attendance is " +
                      std::to_string(static_cast<int>(percentage)) +
                      "% — below the 80% minimum requirement!";
    // Use polymorphism: sendNotification() calls the correct derived class
    // version
    for (auto &notifier : notifiers) {
      notifier->sendNotification(msg);
    }
    return msg;
  }
  return ""; // No warning needed
}
