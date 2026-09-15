#pragma once

enum class FaceExpression {
  Neutral,
  Smile,
};

constexpr FaceExpression faceExpressionForPressed(bool pressed) {
  return pressed ? FaceExpression::Smile : FaceExpression::Neutral;
}
