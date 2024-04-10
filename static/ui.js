function getCookie(name) {
	for (const cookie of document.cookie.split("; ")) {
		const [key, value] = cookie.split("=", 2);
		if (key == name) return value;
	}
}

const me = getCookie("kakeibo_user");
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

uploadForm.onsubmit = (event) => {
	event.preventDefault();
	selectPictureButton.disabled = true;
	takePictureButton.disabled = true;

	fetch("api/upload", {
		method: "POST",
		body: new FormData(uploadForm),
	}).then((response) => {
		if (response.ok)
			return response.json();
		else
			throw new Error("HTTP " + response.status);
	}).then((json) => {
		for (const receipt of json.receipts)
			receiptQueue.push(receipt);
		if (!amountField.value)
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

	dateField.value = receipt.date;
	amountField.value = receipt.total;
	return true;
}

function today() {
	return new Date().toISOString().split("T")[0];
}

entryForm.onreset = () => {
	dateField.defaultValue = today();
	amountField.focus();
};
entryForm.reset();

let lastFocus = Date.now();
document.onfocus = () => {
	const now = Date.now();
	if (now - lastFocus > 3600 && !amountField.value && !remarkField.value)
		entryForm.reset();
	lastFocus = now;
};

/** Bâtit le JSON d’entrée à envoyer à l’API */
function buildEntryData() {
	const data = Object.fromEntries(new FormData(entryForm));
	const amount = Number(data.amount);
	delete data.amount;
	const selectedCategory = entryForm.querySelector("label:has(input[name=category]:checked)");
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

entryForm.onsubmit = () => {
	event.preventDefault();
	submitEntryButton.disabled = true;
	const data = buildEntryData();

	fetch("api/send", {
		method: "POST",
		headers: { "Content-Type": "application/json" },
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
		// Réinitialise seulement partiellement le formulaire pour
		// faciliter l’ajout de reçus similaires.
		amountField.value = null;
		remarkField.value = null;
		if (!popReceipt())
			amountField.focus();
	}).catch((error) => {
		alert(error.message);
	}).finally(() => {
		submitEntryButton.disabled = false;
	})
};

document.forms.bill.onsubmit = (event) => {
	remarkField.value = event.submitter.value;
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
		dateCell.className = "numeric";
		dateCell.innerText = data.date.replace(/^\d+-0?(\d+)-0?(\d+)$/, "$1月$2日");

		const amountCell = document.createElement("td");
		amountCell.className = "numeric";
		amountCell.innerText = amountFormatter.format(data[me]) + "円";
		amountCell.title = data.category;

		const actionsCell = document.createElement("td");
		actionsCell.className = "actions";
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
			headers: { "Content-Type": "application/json" },
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
