#include "../include/Person.h"

// ============================================================
// Person.cpp — Base class implementation
// ============================================================

// Constructor: Initialize id and name
Person::Person(int id, std::string name) : id(id), name(name) {
  std::cout << "[Person] Created: " << name << " (ID: " << id << ")\n";
}

// Destructor
Person::~Person() { std::cout << "[Person] Destroyed: " << name << "\n"; }

// Return the person's ID
int Person::getID() const { return id; }

// Return the person's name
std::string Person::getName() const { return name; }

// Display basic info (can be overridden)
void Person::displayInfo() const {
  std::cout << "ID: " << id << " | Name: " << name << "\n";
}
