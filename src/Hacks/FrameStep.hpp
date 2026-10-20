#pragma once
namespace scro::stepper {
bool frozen();    // Freeze Player or Frame Step is on
bool consume();   // true once per pending step request
void request();
}
