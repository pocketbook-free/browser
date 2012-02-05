;;; GNUTLS-EXTRA --- Guile bindings for GnuTLS-EXTRA.
;;; Copyright (C) 2007  Free Software Foundation
;;;
;;; GNUTLS-EXTRA is free software; you can redistribute it and/or modify
;;; it under the terms of the GNU General Public License as published by
;;; the Free Software Foundation; either version 3 of the License, or
;;; (at your option) any later version.
;;;
;;; GNUTLS-EXTRA is distributed in the hope that it will be useful,
;;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;; GNU General Public License for more details.
;;;
;;; You should have received a copy of the GNU General Public License
;;; along with GNUTLS-EXTRA; if not, write to the Free Software
;;; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
;;; USA.

;;; Written by Ludovic Court�s <ludo@chbouib.org>.


;;;
;;; Exercise the OpenPGP keyring API part of GnuTLS-extra.
;;;

(use-modules (gnutls extra) (gnutls)
             (srfi srfi-1)
             (srfi srfi-4))

(define %raw-keyring-file
  (search-path %load-path "openpgp-keyring.gpg"))

(define %ascii-keyring-file
  (search-path %load-path "openpgp-keyring.asc"))

(define %ids-in-keyring
  ;; The IDs of keys that are in the keyring.
  ;; Change me if you change the keyring file.
  (list '#u8(#x35 #x14 #x5c #xea
             #xa7 #xd9 #x3c #x3f)
        '#u8(#xbd #x57 #x2c #xdc
             #xcc #xc0 #x7c #x35)))

(define (file-size file)
  (stat:size (stat file)))

(define (valid-keyring? file format)
  ;; Return true if FILE contains a valid keyring encoded in FORMAT.
  (let ((raw-keyring (make-u8vector (file-size file))))

    (uniform-vector-read! raw-keyring (open-input-file file))

    (let ((keyring (import-openpgp-keyring raw-keyring format))
          (null-id (make-u8vector 8 0)))

      (and (openpgp-keyring? keyring)
           (not (openpgp-keyring-contains-key-id? keyring null-id))
           (every (lambda (id)
                    (openpgp-keyring-contains-key-id? keyring id))
                  %ids-in-keyring)))))

(dynamic-wind

    (lambda ()
      #t)

    (lambda ()
      (exit
       (every valid-keyring?
              (list %raw-keyring-file
                    %ascii-keyring-file)
              (list openpgp-certificate-format/raw
                    openpgp-certificate-format/base64))))

    (lambda ()
      ;; failure
      (exit 1)))

;;; arch-tag: 516bf608-5c8b-4787-abe9-5f7b6e6d660b
