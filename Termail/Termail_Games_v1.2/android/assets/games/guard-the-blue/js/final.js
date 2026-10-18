const finalStage = document.getElementById('final-stage');
const referencesStage = document.getElementById('references-stage');

function startFinal() {
  document.querySelectorAll('#ending-stage, #gallery-stage, #heart-stage, #result-stage, #choice-stage, #stage')
    .forEach(el => el && el.classList.add('hidden'));
  finalStage.classList.remove('hidden');
}

document.getElementById('show-refs').addEventListener('click', () => {
  referencesStage.classList.remove('hidden');
});

document.getElementById('hide-refs').addEventListener('click', () => {
  referencesStage.classList.add('hidden');
});

document.getElementById('restart-btn').addEventListener('click', () => {
  location.reload();
});