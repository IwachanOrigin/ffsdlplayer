
#ifndef SUBJECT_H_
#define SUBJECT_H_

#include <vector>

namespace player
{

enum class SubjectType
{
  None = 0
  , Reader
  , Decoder
  , Renderer
};

enum class SubjectMode
{
  Primary = 0
  , Secondary
};

class Observer;

class Subject
{

public:
  explicit Subject() = default;
  virtual ~Subject() = default;

  void addObserver(Observer* observer);
  void deleteObserver(Observer* observer);

  SubjectType subjectType() const { return m_subjectType; }

  SubjectMode subjectMode() const { return m_subjectMode; }
  void setSubjectMode(const SubjectMode& mode) { m_subjectMode = mode; }

protected:
  void notifyObservers();
  void setSubjectType(const SubjectType& type) { m_subjectType = type; }

private:
  std::vector<Observer*> m_observers;
  SubjectType m_subjectType = SubjectType::None;
  SubjectMode m_subjectMode = SubjectMode::Primary;
};

} // player

#endif // SUBJECT_H_

