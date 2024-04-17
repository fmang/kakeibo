/*
 * Authentification
 * ----------------
 *
 * On récupère la clé d’API et l’utilisateur courant depuis le fragment de
 * l’URL. Il doit être de la forme `user:api_key`. L’utilisateur ne sert pas à
 * l’authentification mais permet de savoir qui paie. Le vrai utilisateur est
 * noté dans le journal côté API.
 *
 */

const [me, api_key] = document.location.hash.substring(1).split(":");

let you;
switch (me) {
	case 'riku': you = 'anju'; break;
	case 'anju': you = 'riku'; break;
	default: document.location = 'welcome.html';
}

selectPictureButton.onclick = () => {
	pictureSelector.removeAttribute("capture");
	pictureSelector.showPicker();
};

if (pictureSelector.capture) // Supporté par le navigateur.
	takePictureButton.style.display = "inline-flex";

takePictureButton.onclick = () => {
	pictureSelector.setAttribute("capture", "environment");
	pictureSelector.showPicker();
};

const receiptQueue = [];

function updateQueueCounter() {
	queueCounter.innerText = `あと ${receiptQueue.length} 枚`;
	queueCounter.style.display = (receiptQueue.length == 0) ? 'none' : 'inline';
}

uploadForm.onsubmit = (event) => {
	event.preventDefault();
	selectPictureButton.disabled = true;
	takePictureButton.disabled = true;

	fetch("api/upload", {
		method: "POST",
		headers: { 'Authorization': `Bearer ${api_key}` },
		body: new FormData(uploadForm),
	}).then((response) => {
		if (response.ok)
			return response.json();
		else
			throw new Error("HTTP " + response.status);
	}).then((json) => {
		for (const receipt of json.receipts)
			receiptQueue.push(receipt);
		updateQueueCounter();
		if (!entry.amount.value)
			popReceipt();
	}).catch((error) => {
		alert(error.message);
	}).finally(() => {
		selectPictureButton.disabled = false;
		takePictureButton.disabled = false;
	})
};

function popReceipt() {
	const receipt = receiptQueue.shift();
	if (!receipt)
		return false;

	entry.date.value = receipt.date;
	entry.amount.value = receipt.amount;
	entry.remark.value = receipt.remark || null;
	entry.registration.value = receipt.registration || null;
	if (receipt.category)
		entry.category.value = receipt.category;

	updateQueueCounter();
	return true;
}

// Si on reçoit plusieurs reçus avec le même numéro d’inscription, et que le
// premier est sauvegardé avec une catégorie et une remarque, on associe cette
// catégorie et remarque à tous les autres reçus en attente.
function updateReceiptsStore(registration, category, remark) {
	for (const receipt of receiptQueue) {
		if (receipt.registration == registration) {
			receipt.category = category;
			receipt.remark = remark;
		}
	}
}

function today() {
	return new Date().toISOString().split("T")[0];
}

entry.onreset = () => {
	entry.date.defaultValue = today();
	entry.amount.focus();
};
entry.reset();

// Quand l’application est réouverte, on focus le prix car c’est là que tout
// démarre en pratique. Si en plus il s’est passé une heure depuis le dernier
// focus et qu’il n’y a pas de données pré-remplies, on reset le form pour
// avoir la date courante et la catégorie par défaut.
let lastFocus = Date.now();
window.onfocus = () => {
	const now = Date.now();
	if (now - lastFocus > 3600000 && !entry.amount.value && !entry.remark.value)
		entry.reset();
	else if (!entry.amount.value)
		entry.amount.focus();
	lastFocus = now;
};

/** Bâtit le JSON d’entrée à envoyer à l’API */
function buildEntryData() {
	const data = Object.fromEntries(new FormData(entry));
	const amount = Number(data.amount);
	delete data.amount;
	const selectedCategory = entry.querySelector("label:has(input[name=category]:checked)");
	switch (selectedCategory.className) {
		case 'expense':
			data[me] = -amount;
			data[you] = null;
			break;
		case 'income':
			data[me] = amount;
			data[you] = null;
			break;
		case 'transfer':
			data[me] = -amount;
			data[you] = amount;
			break;
		default:
			alert("不正カテゴリー");
			throw("Bad category");
	}
	return data;
}

entry.onsubmit = () => {
	event.preventDefault();
	submitEntryButton.disabled = true;
	const data = buildEntryData();

	fetch("api/send", {
		method: "POST",
		headers: {
			'Authorization': `Bearer ${api_key}`,
			"Content-Type": "application/json",
		},
		body: JSON.stringify(data),
	}).then((response) => {
		if (response.ok)
			return response.json();
		else
			throw new Error("HTTP " + response.status);
	}).then((json) => {
		data.id = json['id'];
		new HistoryEntry(data);
		flyPlane();

		// Partage le magasin avec les autres reçus de la file.
		if (entry.registration.value)
			updateReceiptsStore(entry.registration.value,
			                    entry.category.value,
			                    entry.remark.value);

		// Réinitialise seulement partiellement le formulaire pour
		// faciliter l’ajout de reçus similaires.
		entry.amount.value = null;
		entry.remark.value = null;
		entry.registration.value = null;

		if (!popReceipt())
			entry.amount.focus();
	}).catch((error) => {
		alert(error.message);
	}).finally(() => {
		submitEntryButton.disabled = false;
	})
};

document.forms.bill.onsubmit = (event) => {
	entry.remark.value = event.submitter.value;
	billDialog.close();
	return false;
};

openHistoryButton.onclick = () => {
	historyDialog.showModal();
	openHistoryButton.classList.remove("error");
};

const amountFormatter = Intl.NumberFormat("ja-JP", { signDisplay: "exceptZero" });

class HistoryEntry {
	#row;
	#withdrawButton;

	constructor(data) {
		this.data = data;

		const dateCell = document.createElement("td");
		dateCell.style.textAlign = "right";
		dateCell.innerText = data.date.replace(/^\d+-0?(\d+)-0?(\d+)$/, "$1月$2日");

		const amountCell = document.createElement("td");
		dateCell.style.textAlign = "right";
		amountCell.innerText = amountFormatter.format(data[me]) + "円";
		amountCell.title = data.category;

		const actionsCell = document.createElement("td");
		dateCell.style.textAlign = "right";
		this.#withdrawButton = document.createElement("button");
		this.#withdrawButton.innerText = "取消";
		this.#withdrawButton.onclick = () => { this.withdraw(); };
		actionsCell.appendChild(this.#withdrawButton);

		this.#row = document.createElement("tr");
		this.#row.appendChild(dateCell);
		this.#row.appendChild(amountCell);
		this.#row.appendChild(actionsCell);
		historyTable.appendChild(this.#row);
	}

	withdraw() {
		this.#withdrawButton.disabled = true;

		fetch("api/withdraw", {
			method: "POST",
			headers: {
				'Authorization': `Bearer ${api_key}`,
				"Content-Type": "application/json",
			},
			body: JSON.stringify({ id: this.data.id }),
		}).then((response) => {
			if (response.ok)
				return response.json();
			else
				throw new Error("HTTP " + response.status);
		}).then((json) => {
			this.#row.classList.add("withdrawn");
			this.#withdrawButton.style.visibility = "hidden";
		}).catch((error) => {
			alert(error.message);
			if (!historyDialog.open)
				openHistoryButton.classList.add("error");
			this.#withdrawButton.disabled = false;
		})
	}
}

/*
 * L’avion est une petite icône qui se déplace du bouton d’envoi vers le bouton
 * d’historique, pour offrir un retour visuel. Comme le traitement est
 * asynchrone, le bouton ne se bloque pas.
 */

let plane = null;

function flyPlane() {
	plane?.remove();
	plane = document.createElement("span");
	plane.className = "plane";
	centerTo(plane, submitEntryButton);
	document.body.appendChild(plane);
	centerTo(plane, openHistoryButton);
}

function centerTo(element, reference) {
	with (reference) {
		element.style.position = "absolute";
		element.style.top = offsetTop + offsetHeight / 2 + "px";
		element.style.left = offsetLeft + offsetWidth / 2 + "px";
	}
}
