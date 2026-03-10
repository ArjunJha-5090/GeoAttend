#pragma once
#include <string>
#include <iostream>

// ============================================================
// Person.h — Base class for all people in the system
// Uses encapsulation: id and name are private
// ============================================================

class Person {
private:
    int id;          // Unique ID
    std::string name; // Full name

public:
    // Constructor
    Person(int id, std::string name);

    // Virtual destructor (important for polymorphism)
    virtual ~Person();

    // Getters
    int getID() const;
    std::string getName() const;

    // Virtual display function (can be overridden by child classes)
    virtual void displayInfo() const;
};
